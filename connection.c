#include "jimcore_internal.h"
#include <mysql.h>
#include <mysqld_error.h>

char *host = "dogfood";
char *user = "dogfs";
char *passwd = "";
char *db = "dogfs";
char *unix_socket = NULL;
static unsigned port = 3306;
static int flags = 0;

#define NSEC_PER_SEC (1000 * 1000 * 1000)
#define NSEC_PER_MSEC (1000 * 1000)

void
connection_close(connection_t *c)
{
    int n = SET_COUNT(stmts);
    for (int i = 0; i < n; ++i) {
        if (c->stmts[i].s) {
            if (mysql_stmt_close(c->stmts[i].s))
                vlog("error closing statement");
        }
    }
    mysql_close(c->c);
    free(c);
}

static bool
connection_statement(connection_t *c, struct stmt_def *def)
{
    MYSQL_STMT *s = mysql_stmt_init(c->c);
    if (!s)
        return false;;
    int r = mysql_stmt_prepare(s, def->query, strlen(def->query));
    if (r) {
        vlog("Error preparing `%s': %s", def->query, mysql_error(c->c));
        mysql_stmt_close(s);
        return true;
    }
    c->stmts[def->id].s = s;
    c->stmts[def->id].def = def;;
    return false;
}

static connection_t *
connection_create(MYSQL *m)
{
    int n = SET_COUNT(stmts);
    size_t bytes = sizeof(connection_t) + sizeof(statement_t) * n;
    connection_t *c = malloc(bytes);
    memset(c, 0, bytes);
    c->c = m;
    return c;
}

static bool
connection_init_statements(connection_t *c)
{
    struct stmt_def **defptr;
    SET_FOREACH(defptr, stmts) {
        if (connection_statement(c, *defptr))
            return false;
    }
    return true;
}

connection_t *
connection_open_raw(void)
{
    MYSQL *c = mysql_init(NULL);
    if (mysql_real_connect(c, host, user, passwd, db,
                           port, unix_socket, flags)) {
        vvlog("Opened mysql connection");

        return connection_create(c);
    }
    vlog("Error connecting to database: %s", mysql_error(c));
    mysql_close(c);
    return NULL;
}

connection_t *
connection_open(void)
{
    connection_t *c = connection_open_raw();
    if (!c) {
        return NULL;
    } else if (!connection_init_statements(c)) {
        connection_close(c);
        return NULL;
    } else {
        return c;
    }
}

static struct timespec
gettime(void)
{
    struct timespec tv;
    clock_gettime(CLOCK_MONOTONIC, &tv);
    return tv;
}

static int
statement_error(statement_t *s)
{
    counter_inc(&s->def->errors);
    int err = mysql_stmt_errno(s->s);
    if (err == ER_DUP_ENTRY) {
        return -EEXIST;
    } else {
        vlog("query error for `%s`: %s",
             s->def->query, mysql_stmt_error(s->s));
        return -ETRANSIENT;
    }
}

/* Return time difference in ns */
static unsigned long
timespec_sub_ns(struct timespec *a, struct timespec *b)
{
    unsigned long ns = (a->tv_sec - b->tv_sec) * NSEC_PER_SEC;
    ns += a->tv_nsec;
    ns -= b->tv_nsec;
    return ns;
}

int
_connection_execute(connection_t *c, int id, MYSQL_BIND *params,
                    statement_t **s_out)
{
    int r;
    struct timespec start = gettime();
    c->statement_count++;
    statement_t *s = &c->stmts[id];
    counter_inc(&s->def->calls);
    if (mysql_stmt_bind_param(s->s, params))
        r = statement_error(s);
    else if (mysql_stmt_execute(s->s))
        r = statement_error(s);
    else
        r = 0;
    struct timespec finish = gettime();
    counter_add(&s->def->runtime_ns, timespec_sub_ns(&finish, &start));
    *s_out = s;
    return r;
}

void
connection_error(connection_t *c)
{
    c->errored = true;
}

void
statement_release(statement_t *s)
{
    if (mysql_stmt_free_result(s->s))
        vlog("error freeing result");
}

static void
connection_init(void)
{
    int i = 0;
    struct stmt_def **sptr;
    SET_FOREACH(sptr, stmts) {
        struct stmt_def *s = *sptr;
        s->id = i++;
        counter_init(&s->calls);
        counter_init(&s->errors);
        counter_init(&s->runtime_ns);
    }
}
SET_ENTRY(init_hooks, connection_init);

bool
statement_bind_result(statement_t *s, MYSQL_BIND *row)
{
    if (mysql_stmt_bind_result(s->s, row)) {
        vlog("cannot bind result for `%s': %s",
             s->def->query,  mysql_stmt_error(s->s));
        return true;
    }
    return false;
}

int
statement_fetch(statement_t *s)
{
    int r = mysql_stmt_fetch(s->s);
    if (r == 1)
        vlog("cannot fetch row for `%s': %s",
             s->def->query, mysql_stmt_error(s->s));
    return r;
}

int
connection_begin(connection_t *c)
{
    return run_statement(c, "begin", NULL);
}

int
connection_commit(connection_t *c)
{
    return run_statement(c, "commit", NULL);
}

int
connection_rollback(connection_t *c)
{
    return run_statement(c, "rollback", NULL);
}

int
connection_commit_or_rollback(connection_t *c, int res)
{
    if (res < 0)
        connection_rollback(c);
    else {
        int x = connection_commit(c);
        if (x < 0)
            res = x;
    }
    return res;
}

int
check_update(statement_t *s)
{
    if (mysql_stmt_affected_rows(s->s) == 0)
        return -ENOENT;
    return 0;
}

/* Order by runtime, largest first. */
static int
stmt_def_cmp(const void *x, const void *y)
{
    struct stmt_def *a = *(struct stmt_def**)x;
    struct stmt_def *b = *(struct stmt_def**)y;
    if (counter_value(&a->runtime_ns) > counter_value(&b->runtime_ns))
        return -1;
    if (counter_value(&a->runtime_ns) < counter_value(&b->runtime_ns))
        return 1;
    return 0;
}

static void
statements_dump_stats(void)
{
    log("Prepared statements:");
    int n = SET_COUNT(stmts);
    struct stmt_def *stmts[n];
    struct stmt_def **defptr;
    int i = 0;
    SET_FOREACH(defptr, stmts) {
        stmts[i++] = *defptr;
    }
    qsort(stmts, n, sizeof(struct stmt_def *), stmt_def_cmp);
    for (i = 0; i < n; ++i) {
        struct stmt_def *s = stmts[i];
        log(" %s  [%lu msec/%ld calls/%ld errors]",
            s->query, counter_value(&s->runtime_ns) / NSEC_PER_MSEC,
            counter_value(&s->calls), counter_value(&s->errors));
    }
}
SET_ENTRY(debug_hooks, statements_dump_stats);

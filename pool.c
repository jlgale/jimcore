/* A basic connection pool implementation. */
#include "jimcore_internal.h"

/* Close connections which have not been used in the last
 * CONNECTION_COLLECT_SECONDS. */
#define CONNECTION_COLLECT_SECONDS (5 * 60)

static size_t statements = 0;
static size_t connections = 0;
static size_t acquires = 0;
static size_t errors = 0;
static connection_t *retiring = NULL;
static connection_t *pool = NULL;
static pthread_mutex_t pool_lock = PTHREAD_MUTEX_INITIALIZER;

connection_t *
connection_acquire(void)
{
    connection_t *c = NULL;
    pthread_mutex_lock(&pool_lock);
    if (pool) {
        c = pool;
        pool = c->next;
    } else if (retiring) {
        c = retiring;
        retiring = c->next;
    }
    if (!c)
        connections++;
    acquires++;
    pthread_mutex_unlock(&pool_lock);
    return c ?: connection_open();
}

void
connection_release(connection_t *c)
{
    if (c->errored) {
        pthread_mutex_lock(&pool_lock);
        errors++;
        statements += c->statement_count;
        pthread_mutex_unlock(&pool_lock);
        connection_close(c);
    } else {
        pthread_mutex_lock(&pool_lock);
        c->next = pool;
        pool = c;
        pthread_mutex_unlock(&pool_lock);
    }
}

static void *
connection_collect_loop(void *x)
{
    while (1) {
        sleep(CONNECTION_COLLECT_SECONDS);
        connection_t *closing;
        pthread_mutex_lock(&pool_lock);
        for (connection_t *c = retiring; c; c = c->next)
            statements += c->statement_count;
        closing = retiring;
        retiring = pool;
        pool = NULL;
        pthread_mutex_unlock(&pool_lock);
        while (closing) {
            connection_t *next = closing->next;
            connection_close(closing);
            closing = next;
        }
    }
    return NULL;
}

static void
pool_init(void)
{
    pthread_t t;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&t, &attr, connection_collect_loop, NULL);
}
SET_ENTRY(init_hooks, pool_init);

static void
pool_dump_stats(void)
{
    int open_count = 0;
    count_t statement_count = 0;
    pthread_mutex_lock(&pool_lock);
    for (connection_t *c = pool; c; c = c->next) {
        open_count += 1;
        statement_count += c->statement_count;
    }
    for (connection_t *c = retiring; c; c = c->next) {
        open_count += 1;
        statement_count += c->statement_count;
    }
    statement_count += statements;
    pthread_mutex_unlock(&pool_lock);
    log("Connection pool stats:");
    log("%d open connection(s)", open_count);
    log("%ld statements(s)", statement_count);
    log("%ld acquires(s)", acquires);
    log("%ld connection(s)", connections);
    log("%ld error(s)", errors);
}
SET_ENTRY(debug_hooks, pool_dump_stats);

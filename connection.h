#ifndef _JIMCORE_CONNECTION_H_
#define _JIMCORE_CONNECTION_H_

#include "sys_linker_set.h"

extern char *host;
extern char *user;
extern char *passwd;
extern char *db;
extern char *unix_socket;

/* errno for possibly transient connection / database issues */
#define ETRANSIENT EAGAIN

struct stmt_def
{
    int id;
    char *query;
    counter_t calls;
    counter_t runtime_ns;
    counter_t errors;
};
SET_DECLARE(stmts, struct stmt_def);

typedef struct statement_t statement_t;
struct statement_t
{
    struct stmt_def *def;
    MYSQL_STMT *s;
};

typedef struct connection_t connection_t;
struct connection_t
{
    MYSQL *c;
    connection_t *next; /* next in pool */
    bool errored;
    count_t statement_count;
    statement_t stmts[0];
};

/* Get a full connection. */
connection_t *connection_open(void);
/* A connection, without any prepared statements. */
connection_t *connection_open_raw(void);
/* Close a connection; must be called for every connection_open */
void connection_close(connection_t *c);

int _connection_execute(connection_t *c, int id, MYSQL_BIND *params,
                        statement_t **s_out);

#define connection_execute(__c, __q, __p, __s_out)                      \
    ({                                                                  \
        static struct stmt_def _stmt = { .id = -1, .query = __q };      \
        SET_ENTRY(stmts, _stmt);                                        \
        _connection_execute((__c), _stmt.id, (__p), (__s_out));         \
    })

void connection_error(connection_t *c);
int connection_begin(connection_t *c);
int connection_commit(connection_t *c);
int connection_rollback(connection_t *c);
int connection_query(connection_t *c, char *query);

bool statement_bind_result(statement_t *s, MYSQL_BIND *row);
int statement_fetch(statement_t *s);
void statement_release(statement_t *s);

int connection_commit_or_rollback(connection_t *c, int res);

#define run_with_trx(func, __c, args...)                \
    ({                                                  \
        int _r = connection_begin(__c);                 \
        if (_r == 0) {                                  \
            _r = func(__c, ##args);                     \
            _r = connection_commit_or_rollback(c, _r);  \
        }                                               \
        _r;                                             \
    })


#define run_with_statement(__c, __q, __p, func, args...)                \
    ({                                                                  \
        statement_t *_s;                                                \
        int _r = connection_execute((__c), __q, (__p), &_s);            \
        if (_r == 0) {                                                  \
            _r = func(_s, ##args);                                      \
            statement_release(_s);                                      \
        }                                                               \
        _r;                                                             \
    })

int check_update(statement_t *s);

static inline int
_noop_statement(statement_t *s)
{
    return 0;
}

#define run_with_connection(func, args...)                              \
    ({                                                                  \
        int _r;                                                         \
        connection_t *_c = connection_acquire();                        \
        if (_c) {                                                       \
            _r = func(_c, ##args);                                      \
            if (_r < 0)                                                 \
                connection_error(_c);                                   \
            connection_release(_c);                                     \
        } else {                                                        \
            _r = -ETRANSIENT;                                           \
        }                                                               \
        _r;                                                             \
    })

#define run_statement(__c, __q, __p)                                    \
    run_with_statement(__c, __q, __p, _noop_statement)

#endif

#ifndef _JIMCORE_UTILS_H_
#define _JIMCORE_UTILS_H_

#include "jimcore/connection.h"

/* Connection Pool Functions */
connection_t * connection_acquire(void);
void connection_release(connection_t *c);

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

/* time functions */
static inline void
wallclock(struct timespec *tv)
{
    clock_gettime(CLOCK_REALTIME, tv);
}

/* pthread helpers */
static inline void
_spin_unlock_cleanup(pthread_spinlock_t **lockptr)
{
    pthread_spin_unlock(*lockptr);
}

#define SPIN_LOCK_FOR_SCOPE(_lock)                              \
    pthread_spinlock_t *__locked                                \
    __attribute__((cleanup(_spin_unlock_cleanup))) =            \
         (pthread_spin_lock(&(_lock)), &(_lock))

static inline void
_mutex_unlock_cleanup(pthread_mutex_t **lockptr)
{
    pthread_mutex_unlock(*lockptr);
}

#define MUTEX_LOCK_FOR_SCOPE(_lock)                               \
    pthread_mutex_t *__locked                                     \
    __attribute__((cleanup(_mutex_unlock_cleanup))) =             \
         (pthread_mutex_lock(&(_lock)), &(_lock))

#define LOOP_DECLARE(typ, i...) \
    for (typ *__decl_var = (void *)1, i; __decl_var; __decl_var = NULL)


static inline void
mutex_init(pthread_mutex_t *lock)
{
    static pthread_mutex_t factory = PTHREAD_MUTEX_INITIALIZER;
    *lock = factory;
}

static inline void
thread_create(pthread_t *t, void *(*thread_fn)(void *), void *arg)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(t, &attr, thread_fn, arg);
}

/* Handy bits */
#ifndef MIN
#define MIN(a,b)                \
    ({ typeof(a) _a = (a);      \
       typeof(b) _b = (b);      \
       _a < _b ? _a : _b; })
#endif
#ifndef MAX
#define MAX(a,b)                \
    ({ typeof(a) _a = (a);      \
       typeof(b) _b = (b);      \
       _a > _b ? _a : _b; })
#endif

/* Linker sets */

typedef void (*debug_hook)(void);
SET_DECLARE(debug_hooks, debug_hook);

typedef void (*init_hook)(void);
SET_DECLARE(init_hooks, init_hook);

#endif

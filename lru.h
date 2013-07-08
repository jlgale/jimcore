#ifndef _JIMCORE_LRU_H_
#define _JIMCORE_LRU_H_

#include "jimcore/ring.h"

typedef struct lru_entry_t lru_entry_t;

typedef struct lru_t lru_t;


typedef bool (*lru_equal_fn)(void *a, void *b);
typedef void (*lru_evict_fn)(void *k, void *v);

void lru_init(lru_t *lru,
              size_t limit,
              size_t key_size,
              lru_equal_fn key_equal,
              size_t value_size,
              lru_equal_fn value_equal,
              lru_evict_fn evict);

bool lru_lookup(lru_t *lru, void *key, void **value_out);
void lru_add(lru_t *lru, void *key, void *value);
void lru_remove(lru_t *lru, void *remove);
void lru_remove_value(lru_t *lru, void *remove);
void lru_dump_stats(lru_t *lru);

/* Implementation */

struct lru_entry_t
{
    ring_t lru;
    size_t hits;
    char payload[0];
};

struct lru_t
{
    pthread_spinlock_t lock;
    ring_t lru;
    size_t elements;

    /* Statistics */
    size_t lookups;
    size_t hits;

    /* Static properties. */
    size_t limit;
    size_t key_size;
    size_t value_size;
    lru_equal_fn key_equal;
    lru_equal_fn value_equal;
    lru_evict_fn evict;
};

void *lru_entry_key(lru_t *lru, lru_entry_t *e);
void *lru_entry_value(lru_t *lru, lru_entry_t *e);

#define lru_foreach(_e, _lru)                                           \
    LOOP_DECLARE(pthread_spinlock_t, *__locked                          \
                 __attribute__((cleanup(_spin_unlock_cleanup))) =       \
                 (pthread_spin_lock(&(_lru)->lock), &(_lru)->lock))     \
    ring_foreach_mutable_decl(&(_lru)->lru, lru, lru_entry_t, _e)

#endif

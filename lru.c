/* A simple LRU cache. */

#include "jimcore_internal.h"

#define K(_lru, _v) ((void*)_v->payload)
#define V(_lru, _v) ((void*)_v->payload + (_lru)->key_size)

void
lru_init(lru_t *lru, size_t limit, size_t key_size,
         lru_equal_fn key_equal, size_t value_size,
         lru_equal_fn value_equal, lru_evict_fn evict)
{
    pthread_spin_init(&lru->lock, 0);
    ring_init(&lru->lru);
    lru->limit = limit;
    lru->elements = 0;
    lru->lookups = 0;
    lru->hits = 0;
    lru->key_size = key_size;
    lru->key_equal = key_equal;
    lru->value_size = value_size;
    lru->value_equal = value_equal;
    lru->evict = evict;
}

static lru_entry_t *
lru_lookup_locked_impl(lru_t *lru, void *key)
{
    ring_foreach_mutable_decl(&lru->lru, lru, lru_entry_t, e) {
        if (lru->key_equal(K(lru, e), key)) {
            ring_delete(lru, e);
            ring_prepend(&lru->lru, lru, e);
            return e;
        }
    }
    return NULL;
}

static lru_entry_t *
lru_lookup_locked(lru_t *lru, void *key)
{
    lru->lookups++;
    lru_entry_t *e = lru_lookup_locked_impl(lru, key);
    if (e)
        e->hits++;
    return e;
}

bool
lru_lookup(lru_t *lru, void *key, void **value)
{
    bool found = false;
    pthread_spin_lock(&lru->lock);
    lru_entry_t *e = lru_lookup_locked(lru, key);
    if (e) {
        memcpy(value, V(lru, e), lru->value_size);
        found = true;
    }
    pthread_spin_unlock(&lru->lock);
    return found;
}

static void
lru_evict_impl(lru_t *lru, ring_t *evict)
{
    ring_foreach_mutable_decl(evict, lru, lru_entry_t, e) {
        ring_delete(lru, e);
        lru->evict(K(lru, e), V(lru, e));
        free(e);
    }
}

static void
lru_unlink_impl(lru_t *lru, lru_entry_t *e)
{
    ring_delete(lru, e);
    lru->elements--;
    lru->hits += e->hits;
}

static void
lru_collect_lru(lru_t *lru, lru_entry_t *added, ring_t *evict)
{
    lru_entry_t *e = ring_get_tail(&lru->lru, lru, lru_entry_t);
    while (e && lru->elements > lru->limit) {
        lru_entry_t *p = ring_prev(&lru->lru, lru, lru_entry_t, e);
        if (e != added) {
            lru_unlink_impl(lru, e);
            ring_append(evict, lru, e);
        }
        e = p;
    }
}

static void
lru_add_impl(lru_t *lru, lru_entry_t *e)
{
    ring_t evict;
    ring_init(&evict);
    pthread_spin_lock(&lru->lock);
    ring_prepend(&lru->lru, lru, e);
    lru->elements++;
    lru_collect_lru(lru, e, &evict);
    pthread_spin_unlock(&lru->lock);
    lru_evict_impl(lru, &evict);
}

void
lru_add(lru_t *lru, void *key, void *value)
{
    lru_entry_t *e = malloc(sizeof(*e) + lru->key_size + lru->value_size);
    e->hits = 0;
    memcpy(K(lru, e), key, lru->key_size);
    memcpy(V(lru, e), value, lru->value_size);
    lru_add_impl(lru, e);
}

typedef bool (*lru_remove_test)(lru_entry_t *e);
static void
lru_remove_impl(lru_t *lru, lru_remove_test test)
{
    ring_t found;
    ring_init(&found);
    pthread_spin_lock(&lru->lock);
    ring_foreach_mutable_decl(&lru->lru, lru, lru_entry_t, e) {
        if (test(e)) {
            lru_unlink_impl(lru, e);
            ring_append(&found, lru, e);
        }
    }
    pthread_spin_unlock(&lru->lock);
    lru_evict_impl(lru, &found);
}

void
lru_remove(lru_t *lru, void *key)
{
    auto bool lru_remove_test_key(lru_entry_t *e);
    bool lru_remove_test_key(lru_entry_t *e)
    {
        return lru->key_equal(K(lru, e), key);
    }
    lru_remove_impl(lru, lru_remove_test_key);
}

void
lru_remove_value(lru_t *lru, void *value)
{
    auto bool lru_remove_test_value(lru_entry_t *e);
    bool lru_remove_test_value(lru_entry_t *e)
    {
        return lru->value_equal(V(lru, e), value);
    }
    lru_remove_impl(lru, lru_remove_test_value);
}

void
lru_dump_stats(lru_t *lru)
{
    size_t hit_count = 0;
    pthread_spin_lock(&lru->lock);
    ring_foreach_mutable_decl(&lru->lru, lru, lru_entry_t, e)
        hit_count += e->hits;
    hit_count += lru->hits;
    pthread_spin_unlock(&lru->lock);
    log("cache stats: %ld entries/%ld lookups/%ld hits",
        lru->elements, lru->lookups, hit_count);
}

void *
lru_entry_key(lru_t *lru, lru_entry_t *e)
{
    return K(lru, e);
}

void *
lru_entry_value(lru_t *lru, lru_entry_t *e)
{
    return V(lru, e);
}

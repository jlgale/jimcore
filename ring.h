#ifndef _JIMCORE_RING_H_
#define _JIMCORE_RING_H_

#include <stddef.h>

typedef struct ring {
    struct ring *prev;
    struct ring *next;
} ring_t;

static inline void
ring_init(ring_t *e)
{
    e->next = e;
    e->prev = e;
}

#define ring_static_init(r) r = { &r, &r }

static inline bool
ring_is_empty(ring_t *e)
{
    return e->next == e;
}

static inline void
_ring_append(ring_t *head, ring_t *e)
{
    e->next = head;
    e->prev = head->prev;
    head->prev->next = e;
    head->prev = e;
}

#define ring_append(l, n, e)                            \
    _ring_append(l, &e->n)

static inline void
_ring_prepend(ring_t *head, ring_t *e)
{
    e->next = head->next;
    e->prev = head;
    head->next->prev = e;
    head->next = e;
}

#define ring_prepend(l, n, e)                   \
    _ring_prepend(l, &e->n)

#define ring_entry(n, t, e)                     \
    ((t *)((char *)(e) - offsetof(t, n)))

#define ring_get_head(l, n, t)                                  \
  (ring_is_empty(l) ? NULL : ring_entry(n, t, (l)->next))

#define ring_get_tail(l, n, t)                                    \
    (ring_is_empty(l) ? NULL : ring_entry(n, t, (l)->prev))

#define ring_next(l, n, t, e)                                           \
    ((e)->n.next == (l) ? NULL : ring_entry(n, t, (e)->n.next))

#define ring_prev(l, n, t, e)                                   \
    ((e)->n.prev == (l) ? NULL : ring_entry(n, t, e->n.prev))

static inline void
_ring_delete(ring_t *e)
{
    e->prev->next = e->next;
    e->next->prev = e->prev;
    ring_init(e);
}

#define ring_delete(n, e)                       \
    _ring_delete(&e->n)


#define ring_foreach_decl(l, n, t, e)                                   \
    for (t *e = ring_get_head((l), n, t);                               \
         e;                                                             \
         e = ring_next((l), n, t, e))

#define ring_foreach_mutable_decl(l, n, t, e)                           \
    for (t *e = ring_get_head((l), n, t),                               \
             *__nxt = e ? ring_next((l), n, t, e) : NULL;               \
         e;                                                             \
         e = __nxt, __nxt = ring_next((l), n, t, __nxt))

#endif

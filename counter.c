#include <pthread.h>
#include "counter.h"


void
counter_add(counter_t *c, count_t add)
{
    pthread_spin_lock(&c->lock);
    c->value += add;
    pthread_spin_unlock(&c->lock);
}

void
counter_inc(counter_t *c)
{
    counter_add(c, 1);
}

count_t
counter_value(counter_t *c)
{
    return c->value;
}

void
counter_init(counter_t *c)
{
    pthread_spin_init(&c->lock, 0);
    c->value = 0;
}

void
counter_destroy(counter_t *c)
{
    pthread_spin_destroy(&c->lock);
}

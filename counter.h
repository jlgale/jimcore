#ifndef _JIMCORE_COUNTER_H_
#define _JIMCORE_COUNTER_H_

typedef long count_t;
typedef struct counter_t counter_t;

void counter_init(counter_t *c);
void counter_destroy(counter_t *c);
void counter_inc(counter_t *c);
void counter_add(counter_t *c, count_t add);
count_t counter_value(counter_t *c);

/* Implementation */
struct counter_t
{
    pthread_spinlock_t lock;
    count_t value;
};

#endif

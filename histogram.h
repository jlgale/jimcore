#ifndef _JIMCORE_HISTOGRAM_H_
#define _JIMCORE_HISTOGRAM_H_

#define HISTOGRAM_BUCKETS (64)

typedef struct histogram_t histogram_t;
struct histogram_t
{
    pthread_spinlock_t lock;
    count_t buckets[HISTOGRAM_BUCKETS];
};

void histogram_init(histogram_t *h);
void histogram_add(histogram_t *h, unsigned long v);
void histogram_dump(histogram_t *h);

#endif

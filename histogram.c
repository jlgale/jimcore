#include "jimcore_internal.h"

void
histogram_init(histogram_t *h)
{
    pthread_spin_init(&h->lock, 0);
    for (int i = 0; i < HISTOGRAM_BUCKETS; ++i)
        h->buckets[i] = 0;
}

void
histogram_add(histogram_t *h, unsigned long val)
{
    int b = 8 * sizeof(val) - __builtin_clzl(val);
    assert(b >= 0 && b < HISTOGRAM_BUCKETS);
    SPIN_LOCK_FOR_SCOPE(h->lock);
    h->buckets[b]++;
}

void
histogram_dump(histogram_t *h)
{
    bool print = false;
    SPIN_LOCK_FOR_SCOPE(h->lock);
    for (int i = HISTOGRAM_BUCKETS - 1; i >= 0; --i) {
        if (h->buckets[i])
            print = true;
        if (print)
            log("  < %9lu: %9lu", 1UL << i, h->buckets[i]);
    }
}

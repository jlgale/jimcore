#include "jimcore_internal.h"
#include <signal.h>

static void
debug_handler(int x)
{
    debug_hook **hptr;
    SET_FOREACH(hptr, debug_hooks) {
        debug_hook *x = *hptr;
        debug_hook h = (debug_hook)x;
        h();
    }
}

static void
debug_init(void)
{
    signal(SIGQUIT, debug_handler);
}
SET_ENTRY(init_hooks, debug_init);

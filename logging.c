#include <stdbool.h>
#include "logging.h"

char *argv0;
bool daemonized = true;
bool debug = false;
enum log_level log_level = LOG_LEVEL_ERROR;

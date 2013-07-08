#ifndef _JIMCORE_LOGGING_H_
#define _JIMCORE_LOGGING_H_

enum log_level {
    LOG_LEVEL_FATAL,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
};

extern char *argv0;
extern bool daemonized;
extern bool debug;
extern enum log_level log_level;

#define _log_stdout(lev, fmt, args...)                          \
    do {                                                        \
        if (lev == LOG_LEVEL_FATAL)                             \
            fprintf(stderr, "%s: " fmt "\n", argv0, ##args);    \
        else                                                    \
            fprintf(stdout, "-- " fmt "\n", ##args);            \
    } while (0)

#define _log_syslog(lev, fmt, args...)                  \
    do {                                                \
        int priority = LOG_ALERT;                       \
        switch (lev) {                                  \
        case LOG_LEVEL_DEBUG:                           \
            priority = LOG_DEBUG;                       \
            break;                                      \
        case LOG_LEVEL_INFO:                            \
            priority = LOG_INFO;                        \
            break;                                      \
        case LOG_LEVEL_ERROR:                           \
            priority = LOG_ERR;                         \
            break;                                      \
        case LOG_LEVEL_FATAL:                           \
            priority = LOG_ALERT;                       \
            break;                                      \
        }                                               \
        syslog(priority, fmt, ##args);                  \
    } while (0)                                         \

#define _log_impl(lev, fmt, args...)            \
    do {                                        \
        if (daemonized)                         \
            _log_syslog(lev, fmt, ##args);      \
        else                                    \
            _log_stdout(lev, fmt, ##args);      \
    } while (0)

#ifdef LOG_WITH_POSITION
#define _log(lev, fmt, args...)                                         \
    _log_impl(lev, "%s:%d %s(): " fmt,                                  \
              __FILE__, __LINE__, __FUNCTION__, ##args)
#else
#define _log(lev, fmt, args...)                 \
    _log_impl(lev, fmt, ##args)
#endif

#define log_err(fmt, args...)                   \
    _log(LOG_LEVEL_ERROR, fmt, ##args)

#define log(fmt, args...)                       \
    _log(LOG_LEVEL_INFO, fmt, ##args)

#define vlog(fmt, args...)                      \
    do {                                        \
        if (log_level > LOG_LEVEL_FATAL)        \
            _log(LOG_LEVEL_DEBUG, fmt, ##args); \
    } while (0)

#define vvlog(fmt, args...)                     \
    do {                                        \
        if (log_level > LOG_LEVEL_ERROR)        \
            _log(LOG_LEVEL_DEBUG, fmt, ##args); \
    } while (0)

#define fatal(fmt, args...)                                     \
    do {                                                        \
        _log(LOG_LEVEL_FATAL, fmt, ##args);                     \
        if (debug)                                              \
            abort();                                            \
        _exit(1);                                               \
    } while (0)

#endif

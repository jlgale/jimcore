#ifndef _JIMCORE_MYSQL_HELPERS_H_
#define _JIMCORE_MYSQL_HELPERS_H_

static inline void
bind_int(MYSQL_BIND *bind, int *val)
{
    memset(bind, 0, sizeof(*bind));
    bind->buffer = val;
    bind->buffer_type = MYSQL_TYPE_LONG;
}

static inline void
bind_uint(MYSQL_BIND *bind, unsigned *val)
{
    memset(bind, 0, sizeof(*bind));
    bind->buffer = val;
    bind->buffer_type = MYSQL_TYPE_LONG;
    bind->is_unsigned = true;
}

static inline void
bind_long(MYSQL_BIND *bind, long *val)
{
    memset(bind, 0, sizeof(*bind));
    bind->buffer = val;
    bind->buffer_type = MYSQL_TYPE_LONGLONG;
}

static inline void
bind_ulong(MYSQL_BIND *bind, unsigned long *val)
{
    memset(bind, 0, sizeof(*bind));
    bind->buffer = val;
    bind->buffer_type = MYSQL_TYPE_LONGLONG;
    bind->is_unsigned = true;
}

static inline void
bind_string_len(MYSQL_BIND *bind, char *str, size_t len)
{
    memset(bind, 0, sizeof(*bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = str;
    bind[0].buffer_length = len;
}

static inline void
bind_string(MYSQL_BIND *bind, const char *str)
{
    bind_string_len(bind, (char *)str, strlen(str));
}

#endif

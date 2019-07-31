#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "zs_logger.h"


#define zs_sprintf(buffer, length, fmt) \
    va_list args; \
    va_start(args, (fmt)); \
    length = vsnprintf(buffer, length, fmt, args); \
    va_end(args);


#define _zs_do_log(logger, level, fmt)  \
    do { \
        char    buffer[4000] = ""; \
        int     length = sizeof(buffer) - 1; \
        zs_sprintf(buffer, length, fmt); \
        ztl_log2(logger->zlog, level, buffer, length); \
    } while (0);


static void _zs_log_trace(zs_log_t* logger, const char* fmt, ...)
{
    if (ztl_log_get_level(logger->zlog) > ZTL_LOG_TRACE)
        return;

    _zs_do_log(logger, ZTL_LOG_TRACE, fmt);
}

static void _zs_log_debug(zs_log_t* logger, const char* fmt, ...)
{
    if (ztl_log_get_level(logger->zlog) > ZTL_LOG_DEBUG)
        return;

#if 0
    char    buffer[4000] = "";
    int     length = sizeof(buffer) - 1;
    zs_sprintf(buffer, length, fmt);

    ztl_log2(logger->zlog, ZTL_LOG_WARN, buffer, length);
#else
    _zs_do_log(logger, ZTL_LOG_DEBUG, fmt);
#endif
}

static void _zs_log_info(zs_log_t* logger, const char* fmt, ...)
{
    if (ztl_log_get_level(logger->zlog) > ZTL_LOG_INFO)
        return;

    _zs_do_log(logger, ZTL_LOG_INFO, fmt);
}

static void _zs_log_notice(zs_log_t* logger, const char* fmt, ...)
{
    if (ztl_log_get_level(logger->zlog) > ZTL_LOG_NOTICE)
        return;

    _zs_do_log(logger, ZTL_LOG_NOTICE, fmt);
}

static void _zs_log_warn(zs_log_t* logger, const char* fmt, ...)
{
    if (ztl_log_get_level(logger->zlog) > ZTL_LOG_WARN)
        return;

    _zs_do_log(logger, ZTL_LOG_WARN, fmt);
}

static void _zs_log_error(zs_log_t* logger, const char* fmt, ...)
{
    if (ztl_log_get_level(logger->zlog) > ZTL_LOG_ERROR)
        return;

    _zs_do_log(logger, ZTL_LOG_ERROR, fmt);
}

static void _zs_log_critical(zs_log_t* logger, const char* fmt, ...)
{
    if (ztl_log_get_level(logger->zlog) < ZTL_LOG_CRITICAL)
        return;

    _zs_do_log(logger, ZTL_LOG_CRITICAL, fmt);
}


zs_log_t* zs_log_create(const char* filename, ztl_log_output_t out_type, bool is_async)
{
    zs_log_t* logger;

    logger = malloc(sizeof(zs_log_t));

    logger->zlog    = ztl_log_create(filename, out_type, is_async);
    logger->trace   = _zs_log_trace;
    logger->debug   = _zs_log_debug;
    logger->info    = _zs_log_info;
    logger->notice  = _zs_log_notice;
    logger->warn    = _zs_log_warn;
    logger->error   = _zs_log_error;
    logger->critical= _zs_log_critical;

    return logger;
}

void zs_log_close(zs_log_t* logger)
{
    if (logger)
    {
        if (logger->zlog)
            ztl_log_close(logger->zlog);

        free(logger);
    }
}

// set log level
void zs_log_set_level(zs_log_t* logger, ztl_log_level_t level)
{
    ztl_log_set_level(logger->zlog, level);
}

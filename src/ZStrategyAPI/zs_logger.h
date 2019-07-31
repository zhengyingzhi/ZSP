/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the zs logger
 */

#ifndef _ZS_LOGGER_H_INCLUDED_
#define _ZS_LOGGER_H_INCLUDED_

#include <ZToolLib/ztl_logger.h>


#ifdef __cplusplus
extern "C" {
#endif


/* zs logger object */
typedef struct zs_log_s zs_log_t;
struct zs_log_s
{
    ztl_log_t*  zlog;

    void (*trace)(zs_log_t* logger, const char* fmt, ...);
    void (*debug)(zs_log_t* logger, const char* fmt, ...);
    void (*info)(zs_log_t* logger, const char* fmt, ...);
    void (*notice)(zs_log_t* logger, const char* fmt, ...);
    void (*warn)(zs_log_t* logger, const char* fmt, ...);
    void (*error)(zs_log_t* logger, const char* fmt, ...);
    void (*critical)(zs_log_t* logger, const char* fmt, ...);
};


#if 0
#define zs_log_create       ztl_log_create
#define zs_log_close        ztl_log_close
#define zs_log_set_level    ztl_log_set_level
#endif//0

// create logger
zs_log_t* zs_log_create(const char* filename, ztl_log_output_t out_type, bool is_async);

// release the logger
void zs_log_close(zs_log_t* logger);

// set log level
void zs_log_set_level(zs_log_t* logger, ztl_log_level_t level);

// log message
#define zs_log_trace(log, ...)     ztl_log((log)->zlog, ZTL_LOG_TRACE, __VA_ARGS__)
#define zs_log_debug(log, ...)     ztl_log((log)->zlog, ZTL_LOG_DEBUG, __VA_ARGS__)
#define zs_log_info(log, ...)      ztl_log((log)->zlog, ZTL_LOG_INFO, __VA_ARGS__)
#define zs_log_notice(log, ...)    ztl_log((log)->zlog, ZTL_LOG_NOTICE, __VA_ARGS__)
#define zs_log_warn(log, ...)      ztl_log((log)->zlog, ZTL_LOG_WARN, __VA_ARGS__)
#define zs_log_error(log, ...)     ztl_log((log)->zlog, ZTL_LOG_ERROR, __VA_ARGS__)
#define zs_log_critical(log, ...)  ztl_log((log)->zlog, ZTL_LOG_CRITICAL, __VA_ARGS__)


#ifdef __cplusplus
}
#endif

#endif//_ZS_LOGGER_H_INCLUDED_

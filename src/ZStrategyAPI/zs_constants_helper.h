/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * the constants helpers
 */

#ifndef _ZS_CONSTANTS_HELPER_H_
#define _ZS_CONSTANTS_HELPER_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "zs_constants.h"

#ifdef __cplusplus
extern "C" {
#endif


/// is order finished status
// #define is_finished_status(status)  ((status) == ZS_OS_Filled || (status) == ZS_OS_Canceld || (status) == ZS_OS_PartCancled || (status) == ZS_OS_Rejected)
extern int finished_status_table[];
static inline int is_finished_status(ZSOrderStatus status) {
    return finished_status_table[status];
}


/// convert exchagne name as exchange id
ZSExchangeID zs_convert_exchange_name(const char* exchange_name, int len);

/// convert exchagne name as exchange id
const char* zs_convert_exchange_id(ZSExchangeID exchange_id);

/// make internal id for ordersysid, tradeid
static int zs_make_id(char dst_id[], ZSExchangeID exchange_id, const char* src_id)
{
    return sprintf(dst_id, "%d.%s", exchange_id, src_id);
}

#ifdef __cplusplus
}
#endif

#endif//_ZS_CONSTANTS_HELPER_H_

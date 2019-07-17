/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the order struct
 */

#ifndef _ZS_HELPER_H_INCLUDED_
#define _ZS_HELPER_H_INCLUDED_

#include <stdint.h>

#include "zs_common.h"

#ifdef __cplusplus
extern "C" {
#endif


/// convert exchagne name as exchange id
ZSExchangeID zs_convert_exchange_name(const char* exchange_name);

/// convert exchagne name as exchange id
const char* zs_convert_exchange_id(ZSExchangeID exchange_id);

#ifdef __cplusplus
}
#endif

#endif//_ZS_HELPER_H_INCLUDED_

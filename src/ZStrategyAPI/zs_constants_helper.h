/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * the constants helpers
 */

#ifndef _ZS_CONSTANTS_HELPER_H_
#define _ZS_CONSTANTS_HELPER_H_

#include <stdint.h>

#include "zs_constants.h"

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

#endif//_ZS_CONSTANTS_HELPER_H_

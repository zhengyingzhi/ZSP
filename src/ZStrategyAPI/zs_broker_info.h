/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the broker info
 */

#ifndef _ZS_BROKER_INFO_H_
#define _ZS_BROKER_INFO_H_

#include <stdint.h>

#include <ZToolLib/ztl_vector.h>

#include "zs_core.h"
#include "zs_hashdict.h"


#ifdef __cplusplus
extern "C" {
#endif

#define ZS_MAX_ADDR_NUM     8

/* broker info
 */
struct zs_broker_info_s
{
    char    BrokerName[16];
    char    BrokerID[8];
    char*   TradeAddr[ZS_MAX_ADDR_NUM];
    char*   MdAddr[ZS_MAX_ADDR_NUM];
};

/* load broker infos by setting file
 */
int zs_broker_info_load(ztl_vector_t* broker_infos, const char* setting_file);

/* find broker info
 */
zs_broker_info_t* zs_broker_find_byid(ztl_vector_t* broker_infos, const char* broker_id);
zs_broker_info_t* zs_broker_find_byname(ztl_vector_t* broker_infos, const char* broker_name);


#ifdef __cplusplus
}
#endif

#endif//_ZS_BROKER_INFO_H_

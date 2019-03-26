/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the broker api entry
 */

#ifndef _ZS_BROKER_ENTRY_H_
#define _ZS_BROKER_ENTRY_H_

#include <stdint.h>

#include "zs_broker_api.h"
#include "zs_configs.h"
#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif

#define ZS_MAX_API_NUM  8

/* broker api entry
 * 负责交易核心与各API的交互
 */
struct zs_broker_s
{
    zs_broker_conf_t    BrokerConf;
    zs_trade_api_t*     TradeApis[ZS_MAX_API_NUM];
    zs_md_api_t*        MdApis[ZS_MAX_API_NUM];

    zs_algorithm_t* Algorithm;
};

zs_broker_t* zs_broker_create(zs_algorithm_t* algo);
void zs_broker_release(zs_broker_t* broker);

/* 注册API? */
int zs_broker_add_tradeapi(zs_broker_t* broker, zs_trade_api_t* tradeapi);
int zs_broker_add_mdapi(zs_broker_t* broker, zs_md_api_t* mdapi);

/* 查找需要交易的API */
zs_trade_api_t* zs_broker_get_tradeapi(zs_broker_t* broker, const char* apiName);
zs_md_api_t* zs_broker_get_mdapi(zs_broker_t* broker, const char* apiName);



extern zs_trade_api_handlers_t td_handlers;
extern zs_md_api_handlers_t md_handlers;

#ifdef __cplusplus
}
#endif

#endif//_ZS_BROKER_ENTRY_H_

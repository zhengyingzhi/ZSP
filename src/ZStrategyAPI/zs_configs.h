/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the global algorithm configuration
 */

#ifndef _ZS_CONFIGS_H_INCLUDED_
#define _ZS_CONFIGS_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_palloc.h>
#include <ZToolLib/ztl_producer_consumer.h>

#include "zs_api_object.h"
#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif


#define ZS_ACCOUNTID_SIZE       16
#define ZS_PASSWORD_SIZE        16
#define ZS_BROKERID_SIZE        8
#define ZS_BROKER_ADDR_SIZE     64
#define ZS_STRATEGYNAME_SIZE    16

struct zs_broker_conf_s
{
    char        ApiName[8];
    char        BrokerID[ZS_BROKERID_SIZE];
    char        UserID[ZS_ACCOUNTID_SIZE];
    char        Password[ZS_PASSWORD_SIZE];
    char        Addr[ZS_BROKER_ADDR_SIZE];
    char*       AuthCode;
};

struct zs_strategy_conf_s
{
    char        Name[ZS_STRATEGYNAME_SIZE];
    char*       LibPath;
    char*       Symbols;
};
typedef struct zs_strategy_conf_s zs_strategy_conf_t;

struct zs_trading_conf_s
{
    zs_broker_conf_t    TradeConf;
    zs_broker_conf_t    MdConf;
    ztl_array_t         StrategyArr;    // zs_strategy_conf_t
};
typedef struct zs_trading_conf_s zs_trading_conf_t;


struct zs_algo_param_s
{
    char            StartDate[16];
    char            EndDate[16];

    ZSRunMode       RunMode;
    ZSDataFrequency DataFrequency;
    ZSProductClass  ProductType;
    double          CapitalBase;
    int             FillPolicy;     // current bar close, next bar open

    ztl_array_t     TradingConf;    // zs_trading_conf_t: td, md, stg configures
};


int zs_configs_load(zs_algo_param_t* algoParam, ztl_pool_t* pool, const char* confFile);

#ifdef __cplusplus
}
#endif

#endif//_ZS_CONFIGS_H_INCLUDED_

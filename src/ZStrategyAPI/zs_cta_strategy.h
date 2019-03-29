/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define cta strategy
 */

#ifndef _ZS_CTA_STRATEGY_H_
#define _ZS_CTA_STRATEGY_H_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>

#include "zs_core.h"

#include "zs_strategy_api.h"


#ifdef __cplusplus
extern "C" {
#endif

/* CTA策略 */
typedef struct zs_cta_strategy_s zs_cta_strategy_t;
struct zs_cta_strategy_s
{
    const char* StrategyNme;
    void*       HLib;
    void*       Instance;
    int32_t     StrategyID;
    uint32_t    StrategyFlag;

    zs_account_t*           Account;

    zs_strategy_engine_t*   StgEngine;
    zs_strategy_api_t*      StgApi;
    zs_asset_finder_t*      AssetFinder;
};

zs_cta_strategy_t* zs_cta_strategy_create(zs_strategy_engine_t* engine);
void zs_cta_strategy_release(zs_cta_strategy_t* strategy);

int zs_cta_order(zs_cta_strategy_t* strategy, zs_order_req_t* order_req);
int zs_cta_cancel(zs_cta_strategy_t* strategy, zs_cancel_req_t* cancel_req);


#ifdef __cplusplus
}
#endif

#endif//_ZS_CTA_STRATEGY_H_

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
    int32_t     StrategyID;     // the strategy id in cta engine
    ZSRunStatus RunStatus;      // 

    char*       StrategySetting;//
    const char* pAccountID;
    const char* pCustomID;

    void*       Instance;       // the instance returned by create

    zs_strategy_engine_t*   Engine;
    zs_strategy_entry_t*    Entry;
    zs_asset_finder_t*      AssetFinder;
    zs_blotter_t*           Blotter;

    // 提供给策略访问的接口
    int (*order)(zs_cta_strategy_t* context, zs_sid_t sid, int order_qty, double order_price, ZSDirection direction, ZSOffsetFlag offset);
    int (*place_order)(zs_cta_strategy_t* context, zs_order_req_t* order_req);
    int (*cancel_order)(zs_cta_strategy_t* context, zs_cancel_req_t* cancel_req);
    int (*cancel_all)(zs_cta_strategy_t* context);
    int (*get_account_position)(zs_cta_strategy_t* context, zs_position_engine_t** pposengine, zs_sid_t sid);
    int (*get_trading_account)(zs_cta_strategy_t* context, zs_account_t** paccount);
    int (*get_open_orders)(zs_cta_strategy_t* context, zs_order_t* open_orders[], int size);
    int (*get_orders)(zs_cta_strategy_t* context, zs_order_t* orders[], int size);
    int (*get_trades)(zs_cta_strategy_t* context, zs_trade_t* trades[], int size);
};

zs_cta_strategy_t* zs_cta_strategy_create(zs_strategy_engine_t* engine);
void zs_cta_strategy_release(zs_cta_strategy_t* strategy);


#ifdef __cplusplus
}
#endif

#endif//_ZS_CTA_STRATEGY_H_

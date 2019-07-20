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

#include "zs_strategy_entry.h"


#ifdef __cplusplus
extern "C" {
#endif

/* CTA策略 */
typedef struct zs_cta_strategy_s zs_cta_strategy_t;
struct zs_cta_strategy_s
{
    int32_t     StrategyID;     // the strategy id in cta engine
    ZSRunStatus RunStatus;      // 

    const char* StrategySetting;//
    char*       pAccountID;
    char*       pCustomID;

    void*       Instance;       // the instance returned by create
    void*       UserData;       // the user data for each strategy instance

    zs_strategy_engine_t*   Engine;
    zs_strategy_entry_t*    Entry;
    zs_blotter_t*           Blotter;
    zs_asset_finder_t*      AssetFinder;

    // 提供给策略访问的接口
    zs_sid_t (*lookup_sid)(zs_cta_strategy_t* context, ZSExchangeID exchangeid, const char* symbol, int len);
    const char* (*lookup_symbol)(zs_cta_strategy_t* context, zs_sid_t sid);
    int (*subscribe)(zs_cta_strategy_t* context, zs_sid_t sid);

    int (*order)(zs_cta_strategy_t* context, zs_sid_t sid, int order_qty, double order_price, ZSDirection direction, ZSOffsetFlag offset);
    int (*place_order)(zs_cta_strategy_t* context, zs_order_req_t* order_req);
    int (*cancel_order)(zs_cta_strategy_t* context, zs_cancel_req_t* cancel_req);
    int (*cancel_all)(zs_cta_strategy_t* context);
    int (*get_account_position)(zs_cta_strategy_t* context, zs_position_engine_t** pposengine, zs_sid_t sid);
    int (*get_strategy_position)(zs_cta_strategy_t* context, zs_position_engine_t** pposengine, zs_sid_t sid);
    int (*get_trading_account)(zs_cta_strategy_t* context, zs_account_t** paccount);
    int (*get_open_orders)(zs_cta_strategy_t* context, zs_order_t* open_orders[], int size);
    int (*get_orders)(zs_cta_strategy_t* context, zs_order_t* orders[], int size);
    int (*get_trades)(zs_cta_strategy_t* context, zs_trade_t* trades[], int size);
    zs_contract_t* (*get_contract)(zs_cta_strategy_t* context, zs_sid_t sid);
};

zs_cta_strategy_t* zs_cta_strategy_create(zs_strategy_engine_t* engine, const char* setting, uint32_t strategy_id);
void zs_cta_strategy_release(zs_cta_strategy_t* strategy);

void zs_cta_strategy_set_entry(zs_cta_strategy_t* strategy, zs_strategy_entry_t* strategy_entry);
void zs_cta_strategy_set_blotter(zs_cta_strategy_t* strategy, zs_blotter_t* blotter);


#ifdef __cplusplus
}
#endif

#endif//_ZS_CTA_STRATEGY_H_

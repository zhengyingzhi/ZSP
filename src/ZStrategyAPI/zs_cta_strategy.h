/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define cta strategy api
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


/* 策略信息 */
typedef struct 
{
    int32_t             StrategyID;
    ZSStrategyTypeType  StrategyType;   // single/multi asset type
    ZSTradingFlag   TradingFlag;
    ZSRunStatus     RunStatus;
    char            CreateTime[24];     // YYYY-mm-dd HH:MM:SS
    char            UpdateTime[24];     // YYYY-mm-dd HH:MM:SS
    char            StrategyName[16];
    char            AccountID[ACCOUNT_ID_LEN];
    char            CustomID[ACCOUNT_ID_LEN];
    // uint32_t        PlaceCount;         // 下单次数
    // uint32_t        CancelCount;        // 撤单次数

    char            Symbol[32];         // the trading symbol, maybe empty if multi asset
    int32_t         LongPos;
    int32_t         ShortPos;

    char            Settings[1];        // 策略配置/策略参数等
}zs_strategy_info_t;


/* CTA策略 */
typedef struct zs_cta_strategy_s zs_cta_strategy_t;
struct zs_cta_strategy_s
{
    int32_t             StrategyID;     // the strategy id in cta engine
    ZSStrategyTypeType  StrategyType;   // the strategy type
    ZSTradingFlag   TradingFlag;    // the strategy trading flag
    ZSRunStatus     RunStatus;      // the strategy running status
    time_t          CreateTime;     // the strategy create time
    time_t          UpdateTime;     // the strategy create time

    const char*     StrategySetting;// the strategy raw setting
    char*           pAccountID;
    char*           pCustomID;
    char*           pStrategyName;

    void*           Instance;       // the instance returned by create
    void*           UserData;       // the user data for each strategy instance

    zs_strategy_engine_t*   Engine;
    zs_strategy_entry_t*    Entry;
    zs_blotter_t*           Blotter;
    zs_position_engine_t*   PosEngine;
    zs_asset_finder_t*      AssetFinder;
    zs_json_t*              SettingJson;
    zs_log_t*               Log;

    // 提供给策略访问的接口
    zs_sid_t (*lookup_sid)(zs_cta_strategy_t* context, ZSExchangeID exchangeid, const char* symbol, int len);
    const char* (*lookup_symbol)(zs_cta_strategy_t* context, zs_sid_t sid, ZSExchangeID* pexchangeid);
    int (*subscribe)(zs_cta_strategy_t* context, zs_sid_t sid);
    int (*subscribe_batch)(zs_cta_strategy_t* context, zs_sid_t sids[], int count);

    // place or cancel order
    int (*order)(zs_cta_strategy_t* context, zs_sid_t sid, int order_qty, double order_price, ZSDirection direction, ZSOffsetFlag offset);
    int (*place_order)(zs_cta_strategy_t* context, zs_order_req_t* order_req);
    int (*cancel_order)(zs_cta_strategy_t* context, int frontid, int sessionid, const char* orderid);
    int (*cancel_order2)(zs_cta_strategy_t* context, zs_cancel_req_t* cancel_req);
    int (*cancel_all)(zs_cta_strategy_t* context);

    // getters
    int (*get_account_position)(zs_cta_strategy_t* context, zs_position_engine_t** ppos_engine, zs_sid_t sid);
    int (*get_strategy_position)(zs_cta_strategy_t* context, zs_position_engine_t** ppos_engine, zs_sid_t sid);
    int (*get_trading_account)(zs_cta_strategy_t* context, zs_account_t** paccount);
    int (*get_open_orders)(zs_cta_strategy_t* context, zs_order_t* open_orders[], int size, zs_sid_t filter_sid);
    int (*get_orders)(zs_cta_strategy_t* context, zs_order_t* orders[], int size, zs_sid_t filter_sid);
    int (*get_trades)(zs_cta_strategy_t* context, zs_trade_t* trades[], int size, zs_sid_t filter_sid);
    zs_contract_t* (*get_contract)(zs_cta_strategy_t* context, zs_sid_t sid);

    // get trading day, flag 0:current, 1:next, -1:prev
    int32_t (*get_trading_day)(zs_cta_strategy_t* context, int flag);

    // get strategy info
    const char* (*get_info)(zs_cta_strategy_t* context);
    // get value from conf setting
    int (*get_conf_val)(zs_cta_strategy_t* context, const char* key, void* val, int size, ZSCType ctype);
    // write strategy log
    int (*write_log)(zs_cta_strategy_t* context, const char* content, ...);
};


/// create cta strategy object
zs_cta_strategy_t* zs_cta_strategy_create(zs_strategy_engine_t* engine, const char* setting, uint32_t strategy_id);
void zs_cta_strategy_release(zs_cta_strategy_t* strategy);

void zs_cta_strategy_set_entry(zs_cta_strategy_t* strategy, zs_strategy_entry_t* strategy_entry);
void zs_cta_strategy_set_blotter(zs_cta_strategy_t* strategy, zs_blotter_t* blotter);


// cta strategy request interface
zs_sid_t zs_cta_lookup_sid(zs_cta_strategy_t* context, ZSExchangeID exchangeid, const char* symbol, int len);
const char* zs_cta_lookup_symbol(zs_cta_strategy_t* context, zs_sid_t sid, ZSExchangeID* pexchangeid);
int zs_cta_subscribe(zs_cta_strategy_t* context, zs_sid_t sid);
int zs_cta_subscribe_batch(zs_cta_strategy_t* context, zs_sid_t sids[], int count);

// order interface
int zs_cta_order(zs_cta_strategy_t* strategy, zs_sid_t sid, int order_qty, double order_price, ZSDirection direction, ZSOffsetFlag offset);
int zs_cta_place_order(zs_cta_strategy_t* strategy, zs_order_req_t* order_req);
// int zs_cta_order_value(zs_cta_strategy_t* strategy, zs_sid_t sid, double value);
// int zs_cta_order_percent(zs_cta_strategy_t* strategy, zs_sid_t sid, double percent);
// int zs_cta_order_target(zs_cta_strategy_t* strategy, zs_sid_t sid, double target);

int zs_cta_cancel(zs_cta_strategy_t* strategy, int frontid, int sessionid, const char* orderid);
int zs_cta_cancel_by_sysid(zs_cta_strategy_t* strategy, ZSExchangeID exchangeid, const char* order_sysid);
int zs_cta_cancel_req(zs_cta_strategy_t* strategy, zs_cancel_req_t* cancel_req);
int zs_cta_cancelex(zs_cta_strategy_t* strategy, zs_order_t* order);
int zs_cta_cancel_all(zs_cta_strategy_t* strategy);


// gettters
int zs_cta_get_account_position(zs_cta_strategy_t* context, zs_position_engine_t** ppos_engine, zs_sid_t sid);
int zs_cta_get_strategy_position(zs_cta_strategy_t* context, zs_position_engine_t** ppos_engine, zs_sid_t sid);
int zs_cta_get_trading_account(zs_cta_strategy_t* context, zs_account_t** paccount);
int zs_cta_get_trades(zs_cta_strategy_t* context, zs_trade_t* trades[], int size, zs_sid_t filter_sid);
int zs_cta_get_open_orders(zs_cta_strategy_t* context, zs_order_t* open_orders[], int size, zs_sid_t filter_sid);
int zs_cta_get_orders(zs_cta_strategy_t* context, zs_order_t* orders[], int size, zs_sid_t filter_sid);
zs_order_t* zs_cta_get_order(zs_cta_strategy_t* context, int frontid, int sessionid, const char* orderid);
zs_order_t* zs_cta_get_order_by_sysid(zs_cta_strategy_t* context, ZSExchangeID exchangeid, const char* order_sysid);
zs_contract_t* zs_cta_get_contract(zs_cta_strategy_t* context, zs_sid_t sid);


// operate trading flag
int zs_cta_strategy_set_trading_flag(zs_cta_strategy_t* strategy, ZSTradingFlag trading_flag);

// print the strategy info
int zs_cta_strategy_print(zs_cta_strategy_t* strategy);

// get trading day, flag 0:current, 1:next, -1:prev
int32_t zs_cta_get_trading_day(zs_cta_strategy_t* strategy, int flag);

// 获取策略信息 FIXME use struct data or not
const char* zs_cta_strategy_get_info(zs_cta_strategy_t* strategy);

// get config value from setting
int zs_cta_get_conf_val(zs_cta_strategy_t* context, const char* key, 
    void* val, int size, ZSCType ctype);

// write log
int zs_cta_write_log(zs_cta_strategy_t* context, const char* content, ...);


#ifdef __cplusplus
}
#endif

#endif//_ZS_CTA_STRATEGY_H_

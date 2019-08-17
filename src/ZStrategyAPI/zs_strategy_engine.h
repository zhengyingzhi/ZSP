/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define strategy engine manager
 */

#ifndef _ZS_STRATEGY_ENGINE_H_
#define _ZS_STRATEGY_ENGINE_H_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_vector.h>

#include "zs_bar_generator.h"
#include "zs_core.h"
#include "zs_cta_strategy.h"
#include "zs_hashdict.h"
#include "zs_logger.h"



#ifdef __cplusplus
extern "C" {
#endif

/* 策略引擎，负责策略的管理
 */
struct zs_strategy_engine_s
{
    zs_algorithm_t*     Algorithm;
    zs_asset_finder_t*  AssetFinder;
    ztl_pool_t*         Pool;
    zs_log_t*           Log;

    ztl_array_t*        StrategyEntries;    // 所有导入的策略

    ztl_array_t*        AllStrategy;        // 所有策略实例 [zs_cta_strategy_t*]
    ztl_map_t*          StrategyMap;        // <strategyid, zs_cta_strategy_t*>
    ztl_dict_t*         Tick2StrategyList;  // <Sid, zs_cta_strategy_t*列表>
    ztl_dict_t*         BarGenDict;         // bar合成器<Sid, ztl_bar_gen_t*>
    zs_orderdict_t*     OrderStrategyDict;  // <order_key, zs_cta_strategy_t*>
    ztl_dict_t*         AccountStrategyDict;// <account, zs_cta_strategy_t*列表>

    ztl_dict_t*         TradeDict;          // 成交ID集合 ExchangeID+TradeID

    ZSDataNotifyType    NotifyType;         // 数据通知类型
    uint32_t            StrategyBaseID;     // 策略编号
    int32_t             TradingDay;         // 交易日
    int32_t             AutoGenBar;         // 自动合成分钟bar
};

/* cta engine related
 */
zs_strategy_engine_t* zs_strategy_engine_create(zs_algorithm_t* algo);
void zs_strategy_engine_release(zs_strategy_engine_t* zse);

void zs_strategy_engine_start(zs_strategy_engine_t* zse);
void zs_strategy_engine_stop(zs_strategy_engine_t* zse);

// process update trading day
void zs_strategy_engine_update_tradingday(zs_strategy_engine_t* zse, int32_t trading_day);

// load strategy entry from dso
int zs_strategy_engine_load(zs_strategy_engine_t* zse, ztl_array_t* libpaths);
int zs_strategy_load(zs_strategy_engine_t* zse, const char* libpath);
int zs_strategy_unload(zs_strategy_engine_t* zse, zs_strategy_entry_t* entry);

// add strategy entry
int zs_strategy_entry_add(zs_strategy_engine_t* zse, zs_strategy_entry_t* strategy_entry);

// get strategy entries
ztl_array_t* zs_strategy_get_entries(zs_strategy_engine_t* zse);
zs_strategy_entry_t* zs_strategy_get_entry(zs_strategy_engine_t* zse, const char* strategy_name);


/* 策略的添加、删除、启动、停止、更新等操作
 */
int zs_strategy_create(zs_strategy_engine_t* zse, zs_cta_strategy_t** pstrategy, 
    const char* strategy_name, const char* setting);
int zs_strategy_add(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy);
int zs_strategy_init(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy);
int zs_strategy_del(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy);
int zs_strategy_start(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy);
int zs_strategy_stop(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy);
int zs_strategy_pause(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, int trading_flag);
int zs_strategy_update(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, const char* new_setting);

int zs_strategy_init_all(zs_strategy_engine_t* zse, const char* accountid);
int zs_strategy_start_all(zs_strategy_engine_t* zse, const char* accountid);
int zs_strategy_stop_all(zs_strategy_engine_t* zse, const char* accountid);
int zs_strategy_del_all(zs_strategy_engine_t* zse, const char* accountid);

// 产生策略更新事件
int zs_strategy_put_event(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy);

// 根据代码ID查找策略
ztl_array_t* zs_strategy_find_by_sid(zs_strategy_engine_t* zse, zs_sid_t sid);

// 根据策略名查找策略
int zs_strategy_find_by_name(zs_strategy_engine_t* zse, const char* strategy_name,
    zs_cta_strategy_t* strategy_array[], int size);

// 根据账号查找策略
ztl_array_t* zs_strategy_find_by_account(zs_strategy_engine_t* zse, const char* accountid);

// 根据策略ID查找策略
zs_cta_strategy_t* zs_strategy_find_byid(zs_strategy_engine_t* zse, uint32_t strategy_id);

// 根据策略Key查找策略
zs_cta_strategy_t* zs_strategy_find(zs_strategy_engine_t* zse, int frontid, int sessionid, const char* orderid);


/* operations for cta strategy
 */
int zs_strategy_engine_save_order(zs_strategy_engine_t* zse, 
    zs_cta_strategy_t* strategy, zs_order_req_t* order_req);

int zs_strategy_subscribe(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, 
    ZSExchangeID exchangeid, const char* symbol);
int zs_strategy_subscribe_bysid(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, zs_sid_t sid);
int zs_strategy_subscribe_bysid_batch(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, zs_sid_t sids[], int count);


#ifdef __cplusplus
}
#endif

#endif//_ZS_STRATEGY_ENGINE_H_

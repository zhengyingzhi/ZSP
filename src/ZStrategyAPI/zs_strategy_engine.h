/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define strategy manager
 */

#ifndef _ZS_STRATEGY_H_INCLUDED_
#define _ZS_STRATEGY_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_vector.h>

#include "zs_core.h"

#include "zs_cta_strategy.h"

#include "zs_hashdict.h"




#ifdef __cplusplus
extern "C" {
#endif

/* 策略管理 */

// deprecated
struct zs_strategy_param_s
{
    const char*     StrategyName;
};
typedef struct zs_strategy_param_s zs_strategy_param_t;

struct zs_strategy_engine_s
{
    zs_algorithm_t*     Algorithm;
    zs_asset_finder_t*  AssetFinder;
    ztl_pool_t*         Pool;
    ztl_array_t         AllStrategy;        // 所有策略
    ztl_map_t*          StrategyMap;        // <代码HashID, zs_cta_strategy_t*列表>
    ztl_dict_t*         Tick2StrategyList;  // <Sid, zs_cta_strategy_t*列表>
    ztl_dict_t*         BarGenDict;         // bar合成器<Sid, ztl_bar_gen_t*>
    ztl_dict_t*         OrderStrategyDict;  // <account, <orderid, zs_cta_strategy_t*>>
    ztl_dict_t*         AccountStrategyDict;// <account, zs_cta_strategy_t*列表>
    uint32_t            StrategyBaseID;

    ztl_dict_t*         SubedSymbols;       // 已订阅的合约行情
    ztl_dict_t*         DailyBarsDict;      // 日线历史行情，可由统一的DataPortal管理
    ztl_dict_t*         MinuteBarsDict;     // 

    ztl_dict_t*         TradeIDDict;        // 成交ID集合 ExchangeID+TradeID

    ztl_vector_t*       StrategyPaths;      // 可支持的策略路径列表
};

zs_strategy_engine_t* zs_strategy_engine_create(zs_algorithm_t* algo);
void zs_strategy_engine_release(zs_strategy_engine_t* zse);

int zs_strategy_engine_load(zs_strategy_engine_t* zse, ztl_array_t* stg_libpaths);

int zs_strategy_load(zs_strategy_engine_t* zse, const char* libpath);
int zs_strategy_unload(zs_strategy_engine_t* zse, const char* strategy_name);

zs_cta_strategy_t* zs_strategy_find(zs_strategy_engine_t* zse, uint32_t strategy_id);


/* 策略的添加、删除、启动、停止、更新等操作
 */
int zs_strategy_add(zs_strategy_engine_t* zse, const char* setting);
int zs_strategy_del(zs_strategy_engine_t* zse, uint32_t strategy_id);
int zs_strategy_init(zs_strategy_engine_t* zse, uint32_t strategy_id);
int zs_strategy_start(zs_strategy_engine_t* zse, uint32_t strategy_id);
int zs_strategy_stop(zs_strategy_engine_t* zse, uint32_t strategy_id);
int zs_strategy_update(zs_strategy_engine_t* zse, uint32_t strategy_id, void* data, int size);

/* 策略交易事件处理
 */
void zs_strategy_on_order_req(zs_strategy_engine_t* zse, zs_order_req_t* order_req, uint64_t* order_id);
void zs_strategy_on_order_rtn(zs_strategy_engine_t* zse, zs_order_t* order);
void zs_strategy_on_trade_rtn(zs_strategy_engine_t* zse, zs_trade_t* trade);
void zs_strategy_on_tick(zs_strategy_engine_t* zse, zs_tick_t* tick);
void zs_strategy_on_tickl2(zs_strategy_engine_t* zse, zs_tickl2_t* tickl2);


#ifdef __cplusplus
}
#endif

#endif//_ZS_STRATEGY_H_INCLUDED_

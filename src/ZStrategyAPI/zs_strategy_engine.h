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

#include "zs_core.h"

#include "zs_cta_strategy.h"


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
    ztl_map_t*          StrategyMap;        // <代码HashID, zs_cta_strategy_t*列表>
    ztl_array_t         AllStrategy;        // 所有策略
    ztl_array_t         StrategyEntries;    // 所有策略
    uint32_t            StrategyBaseID;
};

/* cta engine related
 */
zs_strategy_engine_t* zs_strategy_engine_create(zs_algorithm_t* algo);
void zs_strategy_engine_release(zs_strategy_engine_t* zse);

void zs_strategy_engine_stop(zs_strategy_engine_t* zse);

int zs_strategy_engine_load(zs_strategy_engine_t* zse, ztl_array_t* libpaths);
int zs_strategy_load(zs_strategy_engine_t* zse, const char* libpath);
int zs_strategy_unload(zs_strategy_engine_t* zse, zs_strategy_entry_t* entry);

int zs_strategy_add(zs_strategy_engine_t* zse, zs_cta_strategy_t** pstrategy, 
    const char* strategy_name, const char* setting);
int zs_strategy_del(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy);
int zs_strategy_start(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy);
int zs_strategy_stop(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy);
int zs_strategy_update(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, const char* new_setting);

int zs_strategy_find(zs_strategy_engine_t* zse, uint32_t strategy_id, zs_cta_strategy_t* strategy_array[], int size);

/* operations for cta strategy
 */
int zs_strategy_engine_save_order(zs_strategy_engine_t* zse, 
    zs_cta_strategy_t* strategy, zs_order_req_t* order_req);


#ifdef __cplusplus
}
#endif

#endif//_ZS_STRATEGY_H_INCLUDED_

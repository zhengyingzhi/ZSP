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
    ztl_map_t*          StrategyMap;    // <代码HashID, zs_strategy_api_t*列表>
    ztl_array_t         AllStrategy;    // 所有策略
};

zs_strategy_engine_t* zs_strategy_engine_create(zs_algorithm_t* algo);
void zs_strategy_engine_release(zs_strategy_engine_t* zse);

int zs_strategy_engine_load(zs_strategy_engine_t* zse, ztl_array_t* stgLibPaths);

int zs_strategy_load(zs_strategy_engine_t* zse, const char* libPath);
int zs_strategy_unload(zs_strategy_engine_t* zse, const char* stgName);

int zs_strategy_find(zs_strategy_engine_t* zse, uint32_t sid, zs_strategy_api_t* stgArray[]);


#ifdef __cplusplus
}
#endif

#endif//_ZS_STRATEGY_H_INCLUDED_

/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the global algorithm object
 */

#ifndef _ZS_ALGORITHM_H_INCLUDED_
#define _ZS_ALGORITHM_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_dict.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_palloc.h>
#include <ZToolLib/ztl_producer_consumer.h>

#include "zs_blotter_manager.h"
#include "zs_configs.h"
#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef dict ztl_dict_t;

/* global algorithm */

struct zs_algorithm_s
{
    int32_t                 Running;
    ztl_pool_t*             Pool;
    zs_algo_param_t*        Params;
    zs_data_portal_t*       DataPortal;
    zs_event_engine_t*      EventEngine;    // 事件引擎
    zs_simulator_t*         Simulator;      // 回测模拟器(回测事件的产生与事件驱动)
    zs_blotter_manager_t    BlotterMgr;     // 多账户交易管理
    zs_asset_finder_t*      AssetFinder;    // 合约管理
    zs_strategy_engine_t*   StrategyEngine; // 各个策略的管理
    zs_broker_t*            Broker;         // 经纪商接口
    zs_risk_control_t*      RiskControl;    // 风控
};

zs_algorithm_t* zs_algorithm_create(zs_algo_param_t* algoParam);

void zs_algorithm_release(zs_algorithm_t* algo);

int zs_algorithm_init(zs_algorithm_t* algo);

int zs_algorithm_run(zs_algorithm_t* algo, zs_data_portal_t* dataPortal);

int zs_algorithm_stop(zs_algorithm_t* algo);

int zs_algorithm_result(zs_algorithm_t* algo, ztl_array_t* results);



#ifdef __cplusplus
}
#endif

#endif//_ZS_ALGORITHM_H_INCLUDED_

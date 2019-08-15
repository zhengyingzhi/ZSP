/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the global algorithm object
 */

#ifndef _ZS_ALGORITHM_H_INCLUDED_
#define _ZS_ALGORITHM_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_vector.h>
#include <ZToolLib/ztl_dict.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_palloc.h>
#include <ZToolLib/ztl_producer_consumer.h>

#include "zs_blotter_manager.h"
#include "zs_category_info.h"
#include "zs_configs.h"
#include "zs_core.h"
#include "zs_hashdict.h"
#include "zs_logger.h"


#ifdef __cplusplus
extern "C" {
#endif


/* global algorithm */

struct zs_algorithm_s
{
    int32_t                 Running;
    ztl_pool_t*             Pool;
    zs_algo_param_t*        Params;
    zs_event_engine_t*      EventEngine;    // 事件引擎
    zs_data_portal_t*       DataPortal;     // 统一数据入口
    zs_simulator_t*         Simulator;      // 回测模拟器(回测事件的产生与事件驱动)
    zs_category_t*          Category;       // 静态品种信息
    zs_blotter_manager_t    BlotterMgr;     // 多账户交易管理
    zs_asset_finder_t*      AssetFinder;    // 合约管理
    zs_strategy_engine_t*   StrategyEngine; // 各个策略的管理
    zs_broker_t*            Broker;         // 经纪商接口
    zs_risk_control_t*      RiskControl;    // 风控
    // ztl_array_t*            Instruments;    // instruments mainly for backtest

    zs_log_t*               Log;
};

zs_algorithm_t* zs_algorithm_create(zs_algo_param_t* algo_param);

void zs_algorithm_release(zs_algorithm_t* algo);

int zs_algorithm_init(zs_algorithm_t* algo);

int zs_algorithm_set_instruments(zs_algorithm_t* algo, char* instruments[], int count);

int zs_algorithm_run(zs_algorithm_t* algo, zs_data_portal_t* data_portal);

int zs_algorithm_stop(zs_algorithm_t* algo);

int zs_algorithm_result(zs_algorithm_t* algo, ztl_array_t* results);

zs_blotter_t* zs_get_blotter(zs_algorithm_t* algo, const char* accountid);

// mannuly add some settings
int zs_algorithm_add_strategy_entry(zs_algorithm_t* algo, zs_strategy_entry_t* strategy_entry);

int zs_algorithm_add_strategy(zs_algorithm_t* algo, const char* strategy_setting);

int zs_algorithm_add_account(zs_algorithm_t* algo, const char* account_setting);
int zs_algorithm_add_account2(zs_algorithm_t* algo, const zs_conf_account_t* account_conf);

int zs_algorithm_add_broker_info(zs_algorithm_t* algo, const char* broker_setting);
int zs_algorithm_add_broker_info2(zs_algorithm_t* algo, const zs_conf_broker_t* broker_conf);


// some events for backtest
int zs_algorithm_session_start(zs_algorithm_t* algo, zs_bar_reader_t* current_data);
int zs_algorithm_session_before_trading(zs_algorithm_t* algo, zs_bar_reader_t* current_data);
int zs_algorithm_session_every_bar(zs_algorithm_t* algo, zs_bar_reader_t* current_data);
int zs_algorithm_session_end(zs_algorithm_t* algo, zs_bar_reader_t* current_data);

// get version
const char* zs_version(int* pver);

#ifdef __cplusplus
}
#endif

#endif//_ZS_ALGORITHM_H_INCLUDED_

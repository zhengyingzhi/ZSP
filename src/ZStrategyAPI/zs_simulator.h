/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the backtest simulator engine
 */

#ifndef _ZS_SIMULATOR_H_INCLUDED_
#define _ZS_SIMULATOR_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>

#include "zs_core.h"

#include "zs_broker_api.h"


#ifdef __cplusplus
extern "C" {
#endif

/* 定义回测模拟引擎
 * 持有Canlendar, OHLC, TradeApi, MdApi
 * 由sim_engine的Clock后，生成各种回测事件：触发BEFORE_START, EV_BAR， SESSION_END等
 * 数据给到Slippage撮合，再给到MdApi通知到上层（由BrokerEntry使用EventEngine post给交易核心/策略）
 * Slippage产生的回报，由TradeApi通知到上层（由BrokerEntry使用EventEngine post给交易核心/策略）
 */


struct zs_simulator_s
{
    int64_t                 StartDate;
    int64_t                 EndDate;
    zs_algorithm_t*         Algorithm;          // 全局Algo对象
    zs_data_portal_t*       DataPortal;         // 数据读取统一入口
    zs_trading_calendar_t*  TradingCalendar;    // 交易日历
    zs_slippage_t*          Slippage;           // 由于Simulator管理Slippage的生命周期
    zs_trade_api_t*         TdApi;
    zs_trade_api_handlers_t *TdHandlers;        // 用于从模拟器中接收回测交易回报
    zs_md_api_t*            MdApi;
    zs_md_api_handlers_t*   MdHandlers;         // 用于从模拟器中接收回测行情数据

    int32_t                 Progress;
    int32_t                 Running;
};

zs_simulator_t* zs_simulator_create(zs_algorithm_t* algo);

void zs_simulator_release(zs_simulator_t* simu);

// pass-in api handler to connect simulator-platform_core
void zs_simulator_regist_tradeapi(zs_simulator_t* simu,
    zs_trade_api_t* tdapi, zs_trade_api_handlers_t* tdHandlers);
void zs_simulator_regist_mdapi(zs_simulator_t* simu, 
    zs_md_api_t* mdapi, zs_md_api_handlers_t* mdHandlers);

// stop the simulator
void zs_simulator_stop(zs_simulator_t* simu);

// start running simulator
int zs_simulator_run(zs_simulator_t* simu);


// get trader & md api for backtest
zs_trade_api_t* zs_sim_backtest_trade_api();
zs_md_api_t* zs_sim_backtest_md_api();


#ifdef __cplusplus
}
#endif

#endif//_ZS_SIMULATOR_H_INCLUDED_

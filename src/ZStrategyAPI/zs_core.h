/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define framework objects
 */

#ifndef _ZS_CORE_H_INCLUDED_
#define _ZS_CORE_H_INCLUDED_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "zs_api_object.h"

typedef struct zs_algorithm_s zs_algorithm_t;

typedef struct zs_algo_param_s zs_algo_param_t;

typedef struct zs_asset_finder_s zs_asset_finder_t;

typedef struct zs_broker_s zs_broker_t;

typedef struct zs_blotter_s zs_blotter_t;

typedef struct zs_blotter_manager_s zs_blotter_manager_t;

typedef struct zs_event_engine_s zs_event_engine_t;

typedef struct zs_portfolio_s zs_portfolio_t;

typedef struct zs_data_portal_s zs_data_portal_t;

typedef struct zs_bar_reader_s zs_bar_reader_t;

typedef struct zs_risk_control_s zs_risk_control_t;

typedef struct zs_strategy_entry_s zs_strategy_entry_t;
typedef struct zs_strategy_engine_s zs_strategy_engine_t;

typedef struct zs_slippage_s zs_slippage_t;

typedef struct zs_simulator_s zs_simulator_t;

typedef struct zs_trading_calendar_s zs_trading_calendar_t;


typedef struct zs_trade_api_s zs_trade_api_t;
typedef struct zs_md_api_s zs_md_api_t;

typedef struct zs_trade_api_handlers_s zs_trade_api_handlers_t;
typedef struct zs_md_api_handlers_s zs_md_api_handlers_t;



#ifdef __cplusplus
}
#endif

#endif//_ZS_CORE_H_INCLUDED_

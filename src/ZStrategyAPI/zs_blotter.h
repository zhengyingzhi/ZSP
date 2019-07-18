﻿/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define trading core processes
 */

#ifndef _ZS_BLOTTER_H_INCLUDED_
#define _ZS_BLOTTER_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>

#include "zs_assets.h"

#include "zs_account.h"

#include "zs_broker_entry.h"

#include "zs_commission.h"

#include "zs_core.h"

#include "zs_hashdict.h"

#include "zs_order_list.h"

#include "zs_position.h"

#include "zs_protocol.h"


#ifdef __cplusplus
extern "C" {
#endif

/* 交易核心的调度处理
 * 每个账户都有一个Trading
 */
struct zs_blotter_s
{
    zs_algorithm_t*     Algorithm;
    ztl_pool_t*         Pool;

    // 订单管理
    ztl_dict_t*         OrderDict;
    zs_orderlist_t*     WorkOrderList;

    // 成交管理
    ztl_array_t         Trades;

    // 持仓管理 <sid, zs_positions_t>
    ztl_map_t*          Positions;

    // 资金账户
    zs_account_t*       Account;

    // 投资组合
    zs_portfolio_t*     pPortfolio;

    // 手续费模型
    zs_commission_t*    Commission;

    // 风控
    zs_risk_control_t*  RiskControl;

    // 接口
    zs_trade_api_t*     TradeApi;
    zs_md_api_t*        MdApi;

    int (*handle_order_submit)(zs_blotter_t* blotter, zs_order_req_t* order_req);
    int (*handle_order_returned)(zs_blotter_t* blotter, zs_order_t* order);
    int (*handle_order_trade)(zs_blotter_t* blotter, zs_trade_t* trade);
};


zs_blotter_t* zs_blotter_create(zs_algorithm_t* algo, const char* accountid);

void zs_blotter_release(zs_blotter_t* blotter);

// 下单
int zs_blotter_order(zs_blotter_t* blotter, zs_order_req_t* order_req);
int zs_blotter_quote_order(zs_blotter_t* blotter, zs_quote_order_req_t* quote_req);
int zs_blotter_cancel(zs_blotter_t* blotter, zs_cancel_req_t* cancel_req);

int zs_blotter_save_order(zs_blotter_t* blotter, zs_order_req_t* order_req);

// getters
zs_order_t* zs_get_order_by_sysid(zs_blotter_t* blotter, ZSExchangeID exchange_id, const char* order_sysid);
zs_order_t* zs_get_order_by_id(zs_blotter_t* blotter, int32_t frontid, int32_t sessionid, const char* orderid);

zs_position_engine_t* zs_get_position_engine(zs_blotter_t* blotter, zs_sid_t sid);

// 订单事件
int zs_handle_order_submit(zs_blotter_t* blotter, zs_order_req_t* order_req);
int zs_handle_quote_order_submit(zs_blotter_t* blotter, zs_quote_order_req_t* quote_req);
int zs_handle_order_returned(zs_blotter_t* blotter, zs_order_t* order);
int zs_handle_order_trade(zs_blotter_t* blotter, zs_trade_t* trade);

// 行情事件
int zs_handle_md_bar(zs_algorithm_t* algo, zs_bar_reader_t* bar_reader);
int zs_handle_md_tick(zs_algorithm_t* algo, zs_tick_t* tick);


#ifdef __cplusplus
}
#endif

#endif//_ZS_BLOTTER_H_INCLUDED_

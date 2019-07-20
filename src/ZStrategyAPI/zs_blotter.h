/*
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

    int32_t             TradingDay;

    // 订单管理
    ztl_dict_t*         OrderDict;
    zs_orderlist_t*     WorkOrderList;

    // 成交管理
    ztl_dict_t*         TradeDict;        // 成交ID集合 ExchangeID+TradeID
    ztl_array_t*        TradeArray;

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

    // 交易的策略
    ztl_array_t*        TradingStrategy;

    // 请求接口
    int (*order)(zs_blotter_t* blotter, zs_order_req_t* order_req);
    int (*quote_order)(zs_blotter_t* blotter, zs_quote_order_req_t* quote_req);
    int (*cancel)(zs_blotter_t* blotter, zs_cancel_req_t* cancel_req);
    int (*subscribe)(zs_blotter_t* blotter, zs_subscribe_t* sub_req);

    // 回报事件接口
    int (*handle_order_submit)(zs_blotter_t* blotter, zs_order_req_t* order_req);
    int (*handle_order_returned)(zs_blotter_t* blotter, zs_order_t* order);
    int (*handle_order_trade)(zs_blotter_t* blotter, zs_trade_t* trade);
    int (*handle_tick)(zs_blotter_t* blotter, zs_tick_t* tick);
    int (*handle_bar)(zs_blotter_t* blotter, zs_bar_reader_t* bar_reader);
};


zs_blotter_t* zs_blotter_create(zs_algorithm_t* algo, const char* accountid);

void zs_blotter_release(zs_blotter_t* blotter);

void zs_blotter_stop(zs_blotter_t* blotter);


// 下单
int zs_blotter_order(zs_blotter_t* blotter, zs_order_req_t* order_req);
int zs_blotter_quote_order(zs_blotter_t* blotter, zs_quote_order_req_t* quote_req);
int zs_blotter_cancel(zs_blotter_t* blotter, zs_cancel_req_t* cancel_req);

int zs_blotter_save_order(zs_blotter_t* blotter, zs_order_req_t* order_req);

int zs_blotter_subscribe(zs_blotter_t* blotter, zs_subscribe_t* sub_req);

// working order getters
zs_order_t* zs_get_order_by_sysid(zs_blotter_t* blotter, ZSExchangeID exchange_id, const char* order_sysid);
zs_order_t* zs_get_order_by_id(zs_blotter_t* blotter, int32_t frontid, int32_t sessionid, const char* orderid);

zs_position_engine_t* zs_get_position_engine(zs_blotter_t* blotter, zs_sid_t sid);


// 查询回报事件
// int zs_blotter_handle_invetor(zs_blotter_t* blotter, zs_investor_t* investor);
int zs_blotter_handle_account(zs_blotter_t* blotter, zs_fund_account_t* fund_account);
int zs_blotter_handle_position(zs_blotter_t* blotter, zs_position_t* pos);
int zs_blotter_handle_position_detail(zs_blotter_t* blotter, zs_position_detail_t* pos_detail);
int zs_blotter_handle_qry_order(zs_blotter_t* blotter, zs_order_t* order);
int zs_blotter_handle_qry_trade(zs_blotter_t* blotter, zs_trade_t* trade);
int zs_blotter_handle_timer(zs_blotter_t* blotter, int64_t flag);

// 订单事件
int zs_blotter_handle_order_submit(zs_blotter_t* blotter, zs_order_req_t* order_req);
int zs_blotter_handle_quote_order_submit(zs_blotter_t* blotter, zs_quote_order_req_t* quote_req);
int zs_blotter_handle_order_returned(zs_blotter_t* blotter, zs_order_t* order);
int zs_blotter_handle_order_trade(zs_blotter_t* blotter, zs_trade_t* trade);

// 行情事件
int zs_blotter_handle_tick(zs_blotter_t* blotter, zs_tick_t* tick);
int zs_blotter_handle_bar(zs_blotter_t* blotter, zs_bar_reader_t* bar_reader);


#ifdef __cplusplus
}
#endif

#endif//_ZS_BLOTTER_H_INCLUDED_

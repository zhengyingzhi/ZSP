/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the trading account
 */

#ifndef _ZS_ACCOUNT_H_INCLUDED_
#define _ZS_ACCOUNT_H_INCLUDED_

#include <ZToolLib/ztl_map.h>

#include "zs_common.h"

#include "zs_api_object.h"


#ifdef __cplusplus
extern "C" {
#endif

struct zs_frozen_detail_s
{
    uint64_t    Sid;
    char        UserID[16];
    char        Symbol[32];
    double      Price;
    double      Margin;
    int32_t     Volume;
};
typedef struct zs_frozen_detail_s zs_frozen_detail_t;

struct zs_account_s
{
    zs_fund_account_t   FundAccount;
    char                TradingDay[12];
    ztl_map_t*          FinishedOrders;
    ztl_map_t*          FrozenDetails;  // <sid, list>

    int         MaxMarginSide;
    double      FrozenMargin;
    double      FrozenLongMargin;
    double      FrozenShortMargin;
};

typedef struct zs_account_s zs_account_t;


zs_account_t* zs_account_create();

void zs_account_release(zs_account_t* account);

// 资金更新
void zs_account_update(zs_account_t* account, zs_fund_account_t* fund_account);

// 订单事件
int zs_account_on_order_req(zs_account_t* account, zs_order_req_t* order_req, zs_contract_t* contract);
int zs_account_on_order_rtn(zs_account_t* account, zs_order_t* order, zs_contract_t* contract);
int zs_account_on_order_trade(zs_account_t* account, zs_order_t* order, zs_trade_t* trade, zs_contract_t* contract);

void zs_account_update_margin(zs_account_t* account, double margin);

#ifdef __cplusplus
}
#endif

#endif//_ZS_ACCOUNT_H_INCLUDED_

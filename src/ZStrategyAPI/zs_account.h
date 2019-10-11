/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define the trading account
 */

#ifndef _ZS_ACCOUNT_H_INCLUDED_
#define _ZS_ACCOUNT_H_INCLUDED_

#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_mempool.h>
#include <ZToolLib/ztl_palloc.h>

#include "zs_common.h"
#include "zs_core.h"
#include "zs_api_object.h"


#ifdef __cplusplus
extern "C" {
#endif


// 冻结细节
struct zs_frozen_detail_s
{
    zs_sid_t    Sid;
    char        UserID[16];
    char        Symbol[32];
    double      Price;
    double      Margin;
    int32_t     Volume;
};
typedef struct zs_frozen_detail_s zs_frozen_detail_t;

// 资金账户
struct zs_account_s
{
    zs_fund_account_t   FundAccount;
    char                TradingDay[12];
    char*               AccountID;
    ztl_pool_t*         Pool;
    zs_blotter_t*       Blotter;
    ztl_map_t*          FrozenDetails;  // <sid, list>
    ztl_mempool_t*      FrozenDetailMP;

    int         MaxMarginSide;
    double      FrozenMargin;
    double      FrozenLongMargin;
    double      FrozenShortMargin;
};
typedef struct zs_account_s zs_account_t;


// 创建账户管理对象
zs_account_t* zs_account_create(zs_blotter_t* blotter);

void zs_account_release(zs_account_t* account);

// 资金更新
void zs_account_fund_update(zs_account_t* account, zs_fund_account_t* fund_account);

// 订单事件
int zs_account_handle_order_req(zs_account_t* account, zs_order_req_t* order_req, zs_contract_t* contract);
int zs_account_handle_order_finished(zs_account_t* account, zs_order_t* order, zs_contract_t* contract);
int zs_account_handle_trade_rtn(zs_account_t* account, zs_order_t* order, zs_trade_t* trade, zs_contract_t* contract);


#ifdef __cplusplus
}
#endif

#endif//_ZS_ACCOUNT_H_INCLUDED_

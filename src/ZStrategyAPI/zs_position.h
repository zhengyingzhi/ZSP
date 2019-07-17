/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the position engine 
 */

#ifndef _ZS_POSITION_H_INCLUDED_
#define _ZS_POSITION_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_dlist.h>
#include <ZToolLib/ztl_palloc.h>

#include "zs_api_object.h"
#include "zs_assets.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * 持仓更新
 */
struct zs_position_engine_s
{
    zs_contract_t*  Contract;
    zs_sid_t        Sid;
    int32_t         Multiplier;
    double          PriceTick;
    double          LongMarginRatio;
    double          ShortMarginRatio;

    ztl_pool_t*     Pool;
    ztl_dlist_t*    PositionDetails;

    char            TradingDay[12];
    double          LastPrice;
    double          RealizedPnl;
    uint32_t        TickUpdatedN;

    // 多仓
    int32_t         LongPos;
    int32_t         LongYdPos;
    int32_t         LongTdPos;
    int32_t         LongFrozen;
    int32_t         LongYdFrozen;
    int32_t         LongTdFrozen;
    int32_t         LongAvail;
    double          LongPrice;
    double          LongPnl;
    double          LongMargin;
    double          LongCost;
    int64_t         LongUpdateTime;

    // 空仓
    int32_t         ShortPos;
    int32_t         ShortYdPos;
    int32_t         ShortTdPos;
    int32_t         ShortFrozen;
    int32_t         ShortYdFrozen;
    int32_t         ShortTdFrozen;
    int32_t         ShortAvail;
    double          ShortPrice;
    double          ShortPnl;
    double          ShortMargin;
    double          ShortCost;
    int64_t         ShortUpdateTime;
};

typedef struct zs_position_engine_s zs_position_engine_t;

// 创建持仓管理对象
zs_position_engine_t* zs_position_create(ztl_pool_t* pool, zs_contract_t* contract);

void zs_position_release(zs_position_engine_t* pos);

// 报单更新
int zs_position_on_order_req(zs_position_engine_t* pos, zs_order_req_t* order_req);

// 订单更新
int zs_position_on_order_rtn(zs_position_engine_t* pos, zs_order_t* order);

// 成交更新
double zs_position_on_trade_rtn(zs_position_engine_t* pos, zs_trade_t* trade);


// 最新价格更新
void zs_position_sync_last_price(zs_position_engine_t* pos, double lastpx);

#ifdef __cplusplus
}
#endif

#endif//_ZS_POSITION_H_INCLUDED_

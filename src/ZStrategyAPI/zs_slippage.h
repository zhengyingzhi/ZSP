/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define slippages for backtest
 */

#ifndef _ZS_SLIPPAGE_H_INCLUDED_
#define _ZS_SLIPPAGE_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_dlist.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_mempool.h>

#include "zs_core.h"
#include "zs_api_object.h"

#include "zs_data_portal.h"
#include "zs_trading_calendar.h"


#ifdef __cplusplus
extern "C" {
#endif

/* slippage可提供线程模型（由单独的线程的收到行情数据，并对报单进行撮合），
 * 非线程模式下，由于外部主动调用一次处理接口，根据行情，撮合订单
 * 最后，输出回报数据
 * 注意：这里slippage只有订单处理（不像交易所那样，由撮合引擎根据撮合结果，主动发出处理），
 * 但是，这里可以提供多种引擎，多种滑点方式
 */

typedef enum
{
    ZS_SDT_Order,
    ZS_SDT_QuoteOrder,
    ZS_SDT_Trade
}ZSSlippageDataType;

typedef enum 
{
    ZS_ST_VolumeShare,
    ZS_ST_VolumeFixed
}ZS_SLIPPAGET_TYPE;

typedef enum
{
    ZS_PFF_Open,
    ZS_PFF_Close
}ZS_PRICE_FIELD_FILL;


/* 处理结果回调函数(即订单回报，成交回报等) */
typedef void(*zs_slippage_handler_pt)(zs_slippage_t* slippage, ZSSlippageDataType datatype, void* data, int datasz);

/* 可根据指定不同的成交撮合方式（TODO:需要可快速获取合约信息，如PriceTick,DecimalPoint等） */
typedef struct zs_slippage_model_s zs_slippage_model_t;
struct zs_slippage_model_s
{
    const char*         ModelNme;
    zs_slippage_t*      Slippage;
    ZS_PRICE_FIELD_FILL PriceField;
    double              VolumeLimit;
    double              VolumeForBar;

    // Process how orders get filled, get filled_price, filled_volume
    int (*process_order_by_bar)(zs_slippage_model_t* slippage_model,
            zs_bar_reader_t* bar_reader, zs_order_t* order, 
            double* pfilled_price, int* pfilled_qty);

    int (*process_order_by_tick)(zs_slippage_model_t* slippage_model,
            zs_tick_t* tick, zs_order_t* order,
            double* pfilled_price, int* pfilled_qty);
};

struct zs_slippage_s
{
    ztl_dlist_t*            OrderList;
    ztl_dlist_t*            NewOrders;
    ztl_dlist_t*            CancelRequests;
    ztl_mempool_t*          MemPool;
    zs_slippage_model_t*    SlippageModel;
    zs_slippage_handler_pt  Handler;
    void*                   UserData;           // 通常是 zs_simulator_t
    ZS_PRICE_FIELD_FILL     PriceField;         // 参考成交价格Open/Close

    int32_t                 trading_day;
    uint32_t                order_sysid;
    uint32_t                trade_id;
};


/* create slippage instance */
zs_slippage_t* zs_slippage_create(zs_slippage_handler_pt handler, void* userdata);

void zs_slippage_release(zs_slippage_t* slippage);

void zs_slippage_set_price_field(zs_slippage_t* slippage, ZS_PRICE_FIELD_FILL price_field);

void zs_slippage_set_model(zs_slippage_t* slippage, zs_slippage_model_t* slippage_model);

int zs_slippage_update_tradingday(zs_slippage_t* slippage, int32_t trading_day);


/* 输入/撤销订单 */
int zs_slippage_order(zs_slippage_t* slippage, const zs_order_req_t* order_req);
int zs_slippage_quote_order(zs_slippage_t* slippage, const zs_quote_order_req_t* quote_order_req);
int zs_slippage_cancel(zs_slippage_t* slippage, const zs_cancel_req_t* cancel_req);

/* 回测事件，进行订单处理：撮合/取消等 */
int zs_slippage_session_start(zs_slippage_t* slippage, zs_bar_reader_t* current_data);
int zs_slippage_session_before_trading(zs_slippage_t* slippage, zs_bar_reader_t* current_data);
int zs_slippage_session_every_bar(zs_slippage_t* slippage, zs_bar_reader_t* current_data);
int zs_slippage_session_end(zs_slippage_t* slippage, zs_bar_reader_t* current_data);

int zs_slippage_process_bybar(zs_slippage_t* slippage, zs_bar_t* bar);
int zs_slippage_process_bytick(zs_slippage_t* slippage, zs_tick_t* tick);


#ifdef __cplusplus
}
#endif

#endif//_ZS_SLIPPAGE_H_INCLUDED_

/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define strategy entrys
 */

#ifndef _ZS_STRATEGY_ENTRY_H_
#define _ZS_STRATEGY_ENTRY_H_

#include <stdint.h>

#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif

/* other types */
typedef struct zs_cta_strategy_s zs_cta_strategy_t;

/* 策略接口的基本定义 */
static const char* zs_strategy_entry_names[] = { "create", "release", "on_init", "on_start", "on_stop", "on_update", \
    "before_trading_start", "handle_order", "handle_trade", "handle_bardata", "handle_tickdata", "handle_tickl2data", NULL };


/* 定义策略对象及策略的回调处理函数
 */

struct zs_strategy_entry_s
{
    const char* StrategyNme;
    uint32_t    StrategyFlag;

    // 策略对象的创建与销毁
    void* (*create)(const char* reserve);
    void  (*release)(zs_cta_strategy_t* context);

    // 策略
    void  (*on_init)(zs_cta_strategy_t* context);
    void  (*on_start)(zs_cta_strategy_t* context);
    void  (*on_stop)(zs_cta_strategy_t* context);
    void  (*on_update)(zs_cta_strategy_t* context, void* data, int size);

    void  (*before_trading_start)(zs_cta_strategy_t* context);

    // 成交通知
    void  (*handle_order)(zs_cta_strategy_t* context, zs_order_t* order);
    void  (*handle_trade)(zs_cta_strategy_t* context, zs_trade_t* trade);

    // 行情通知
    void  (*handle_bardata)(zs_cta_strategy_t* context, zs_bar_reader_t* bar_reader);
    void  (*handle_tickdata)(zs_cta_strategy_t* context, zs_tick_t* tick);
    void  (*handle_tickl2data)(zs_cta_strategy_t* context, zs_tickl2_t* tickl2);
};


//////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif//_ZS_STRATEGY_ENTRY_H_

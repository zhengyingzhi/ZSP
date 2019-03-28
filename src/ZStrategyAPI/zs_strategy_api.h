/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define strategy api
 */

#ifndef _ZS_STRATEGY_API_H_
#define _ZS_STRATEGY_API_H_

#include <stdint.h>

#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif

/* other types */
typedef struct zs_cta_strategy_s zs_cta_strategy_t;

/* 策略接口的基本定义 */
static const char* zs_strategy_api_names[] = { "create", "release", "on_init", "on_start", "on_stop", \
    "before_trading_start", "on_order", "on_trade", "on_bardata", "on_tickdata", "on_tickdataL2", NULL };


/* 定义策略对象及策略的回调处理函数
 */

struct zs_strategy_api_s
{
    const char* StrategyNme;
    void*       HLib;
    void*       Instance;
    int32_t     StrategyID;
    uint32_t    StrategyFlag;

    // 策略对象的创建与销毁
    void* (*create)(zs_cta_strategy_t* context, const char* reserve);
    void  (*release)(void* instance);

    // 策略
    void  (*on_init)(void* instance);
    void  (*on_start)(void* instance);
    void  (*on_stop)(void* instance);
    void  (*on_update)(void* instance, void* data, int size);

    void  (*before_trading_start)(void* instance, zs_cta_strategy_t* context);

    // 成交通知
    void  (*on_order)(void* instance, zs_cta_strategy_t* context, zs_order_t* order);
    void  (*on_trade)(void* instance, zs_cta_strategy_t* context, zs_trade_t* trade);

    // 行情通知
    void  (*on_bardata)(void* instance, zs_cta_strategy_t* context, zs_bar_reader_t* barReader);
    void  (*on_tickdata)(void* instance, zs_cta_strategy_t* context, zs_tick_t* tickData);
    void  (*on_tickdataL2)(void* instance, zs_cta_strategy_t* context, zs_l2_tick_t* tickDataL2);
};


//////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif//_ZS_STRATEGY_API_H_

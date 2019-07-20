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



/* 策略导出的入口函数 */
#define zs_strategy_entry_name  "strategy_entry"

typedef int (*zs_strategy_entry_pt)(zs_strategy_entry_t** ppentry);


/* 定义策略对象及策略的回调处理函数
 */
struct zs_strategy_entry_s
{
    const char* StrategyName;   // the strategy name
    const char* Author;         // the strategy author
    const char* Version;        // the strategy version
    uint32_t    Flags;          // the strategy flag
    void*       HLib;           // the dso object

    // 策略对象的创建与销毁
    void* (*create)(zs_cta_strategy_t* context, const char* setting);
    void  (*release)(void* instance, zs_cta_strategy_t* context);

    int   (*is_trading_symbol)(void* instance, zs_cta_strategy_t* context, zs_sid_t sid);

    // 策略
    void  (*on_init)(void* instance, zs_cta_strategy_t* context);
    void  (*on_start)(void* instance, zs_cta_strategy_t* context);
    void  (*on_stop)(void* instance, zs_cta_strategy_t* context);
    void  (*on_update)(void* instance, zs_cta_strategy_t* context, void* data, int size);

    void  (*on_timer)(void* instance, zs_cta_strategy_t* context, int64_t flag);
    void  (*before_trading_start)(void* instance, zs_cta_strategy_t* context);

    // 成交通知
    void  (*handle_order)(void* instance, zs_cta_strategy_t* context, zs_order_t* order);
    void  (*handle_trade)(void* instance, zs_cta_strategy_t* context, zs_trade_t* trade);

    // 行情通知
    void  (*handle_bar)(void* instance, zs_cta_strategy_t* context, zs_bar_reader_t* bar_reader);
    void  (*handle_tick)(void* instance, zs_cta_strategy_t* context, zs_tick_t* tick);
    void  (*handle_tickl2)(void* instance, zs_cta_strategy_t* context, zs_tickl2_t* tickl2);
};


//////////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif

#endif//_ZS_STRATEGY_API_H_

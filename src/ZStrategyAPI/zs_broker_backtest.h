/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define and impl the broker api for backtest
 */

#ifndef _ZS_BROER_BACKTEST_H_
#define _ZS_BROER_BACKTEST_H_

#include <stdint.h>

#include "zs_core.h"

#include "zs_broker_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 把回测模拟成一个标准的经纪商的API接口形式，
 * 内部仅将订单发给slippage，并接收从slippage的回报
 */

extern zs_trade_api_t   bt_tdapi;
extern zs_md_api_t      bt_mdapi;

void* zs_bt_trade_create(const char* str, int reserve);
void zs_bt_trade_release(void* api_instance);
int zs_bt_trade_regist(void* api_instance, zs_trade_api_handlers_t* tdHandlers,
    void* tdCtx, const zs_broker_conf_t* apiConf);
int zs_bt_trade_connect(void* api_instance, void* addr);
int zs_bt_order(void* api_instance, zs_order_req_t* order_req);
int zs_bt_quote_order(void* api_instance, zs_quote_order_req_t* quote_req);
int zs_bt_cancel(void* api_instance, zs_cancel_req_t* cancel_req);
int zs_bt_query(void* api_instance, ZSApiQueryCategory category, void* data, int size);

// create backtest md api instance
void* zs_bt_md_create(const char* str, int reserve);
void zs_bt_md_release(void* api_instance);
int zs_bt_md_regist(void* api_instance, zs_md_api_handlers_t* md_handlers,
    void* mdctx, const zs_broker_conf_t* apiConf);
int zs_bt_md_connect(void* api_instance, void* addr);
int zs_bt_md_login(void* api_instance);
int zs_bt_md_subscribe(void* api_instance, zs_subscribe_t* sub_reqs[], int count);
int zs_bt_md_unsubscribe(void* api_instance, zs_subscribe_t* unsub_reqs[], int count);


#ifdef __cplusplus
}
#endif

#endif//_ZS_BROER_BACKTEST_H_


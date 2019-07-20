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


// simulate the dso entry func
int zs_bt_trade_api_entry(zs_trade_api_t* tdapi);

int zs_bt_md_api_entry(zs_md_api_t* mdapi);


#ifdef __cplusplus
}
#endif

#endif//_ZS_BROER_BACKTEST_H_


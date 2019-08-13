/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define the backtest routine
 */

#ifndef _ZS_BACKTEST_H_INCLUDED_
#define _ZS_BACKTEST_H_INCLUDED_

#include <stdint.h>

#include "zs_core.h"

#include "zs_broker_api.h"

#ifdef __cplusplus
extern "C" {
#endif


struct zs_backtest_s
{
    zs_conf_backtest_t      BacktestConf;
    zs_strategy_entry_t*    StrategyEntry;
    const char*             StrategySetting;
    ztl_array_t*            RawDatas;
    int32_t                 IsTicks;

    /* below objects created internally */
    ztl_pool_t*             Pool;
    zs_algorithm_t*         Algorithm;
};
typedef struct zs_backtest_s zs_backtest_t;

/* 运行一个回测
 * TODO: how to get backtest result
 */
int zs_backtest_run(zs_backtest_t* backtest);

void zs_backtest_release(zs_backtest_t* backtest);


#ifdef __cplusplus
}
#endif

#endif//_ZS_BACKTEST_H_INCLUDED_

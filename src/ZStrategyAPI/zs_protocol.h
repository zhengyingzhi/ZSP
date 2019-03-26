/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the some core object
 */

#ifndef _ZS_PROTOCOL_H_INCLUDED_
#define _ZS_PROTOCOL_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>

#include "zs_configs.h"
#include "zs_core.h"

#include "zs_protocol.h"


#ifdef __cplusplus
extern "C" {
#endif

struct zs_portfolio_s
{
    ztl_map_t*  Positions;
    double      StartingCash;
    double      EndingCash;
    double      EndingValue;

    uint32_t    TradeCount;
    double      TotalProfit;
    double      TotalLoss;
};


#ifdef __cplusplus
}
#endif

#endif//_ZS_PROTOCOL_H_INCLUDED_

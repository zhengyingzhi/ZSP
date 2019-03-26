/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the order struct
 */

#ifndef _ZS_POSITION_TRACKER_H_
#define _ZS_POSITION_TRACKER_H_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_dlist.h>
#include <ZToolLib/ztl_map.h>

#include "zs_core.h"
#include "zs_assets.h"


#ifdef __cplusplus
extern "C" {
#endif

/* 
 * 持仓管理
 */
struct zs_position_tracker_s
{
    // map<sid, zs_positions_t>
    ztl_map_t*          LongPositions;
    ztl_map_t*          ShortPositions;
};
typedef struct zs_position_tracker_s zs_position_tracker_t;

struct zs_position_api_s
{
    zs_sid_t    Sid;
    void*       CtxData;
    int(*on_order_submit)(zs_position_t* pos, zs_order_req_t* order_req);
    int(*on_order_returned)(zs_position_t* pos, zs_order_t* order);
    int(*on_order_trade)(zs_position_t* pos, zs_trade_t* trade);
};



#ifdef __cplusplus
}
#endif

#endif//_ZS_POSITION_TRACKER_H_

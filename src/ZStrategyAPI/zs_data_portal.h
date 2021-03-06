﻿/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyPlatform
 * define the data feed process
 */

#ifndef _ZS_DATA_PORTAL_H_
#define _ZS_DATA_PORTAL_H_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_dict.h>
#include <ZToolLib/ztl_map.h>

#include "zs_api_object.h"
#include "zs_assets.h"
#include "zs_broker_api.h"
#include "zs_core.h"
#include "zs_data_feed.h"
#include "zs_hashdict.h"


#ifdef __cplusplus
extern "C" {
#endif


/* 表示所有的行情数据，并提供丰富的接口访问数据
 * a. 按时间访问
 * b. 按品种访问
 */
struct zs_data_portal_s
{
    int64_t         StartTime;
    int64_t         EndTime;
    ztl_pool_t*     Pool;

    ztl_array_t*    RawDatas;               // 原始数据（可以在回测事件循环时，一次遍历所有相同时间的数据，临时放到一个BarReader/hashIDMap中，然后调用交易核心）
    ztl_dict_t*     Time2Data;              // 方便使用时间找到该时间的所有数据Array，<time,dict>
    ztl_dict_t*     Asset2Data;             // 每个产品和该产品对应的所有bar数据(一个时间序列的array)
    zs_bar_reader_t *BarReader;             // default bar reader

    int32_t         IsTick;
};


struct zs_bar_reader_s
{
    zs_data_portal_t*   DataPortal;
    ZSDataFrequency     DataFrequency;
    uint32_t            Index;
    int64_t             CurrentDt;
    zs_bar_t            Bar;

    bool    (*can_trade)(zs_bar_reader_t* bar_reader, zs_sid_t sid);
    int     (*history)(zs_bar_reader_t* bar_reader, zs_sid_t sid, zs_bar_t* arr[], int size);
    double  (*current)(zs_bar_reader_t* bar_reader, zs_sid_t sid, const char* field);
    double  (*current2)(zs_bar_reader_t* bar_reader, zs_sid_t sid, ZSFieldType field);

    zs_bar_t* (*current_bar)(zs_bar_reader_t* bar_reader, zs_sid_t sid);
};

int zs_bar_reader_init(zs_bar_reader_t* bar_reader, zs_data_portal_t* data_portal);


zs_data_portal_t* zs_data_portal_create();

void zs_data_portal_release(zs_data_portal_t* data_portal);

int zs_data_portal_wrapper(zs_data_portal_t* data_portal, ztl_array_t* raw_datas);


/* getters */
zs_tick_t* zs_data_portal_get_tick(zs_data_portal_t* data_portal, zs_sid_t sid, int64_t dt);

zs_bar_t* zs_data_portal_get_bar(zs_data_portal_t* data_portal, zs_sid_t sid, int64_t dt);

zs_bar_reader_t* zs_data_portal_get_barreader(zs_data_portal_t* data_portal, int64_t dt);

int zs_data_portal_get3(zs_data_portal_t* data_portal, ztl_array_t* dstarr,
    zs_sid_t sid, int64_t startdt, int64_t enddt);


#ifdef __cplusplus
}
#endif

#endif//_ZS_DATA_PORTAL_H_

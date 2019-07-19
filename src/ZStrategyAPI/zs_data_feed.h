/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyPlatform
 * define the data feed process
 */

#ifndef _ZS_DATA_FEED_H_
#define _ZS_DATA_FEED_H_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_dict.h>
#include <ZToolLib/ztl_map.h>

#include "zs_broker_api.h"
#include "zs_core.h"



#ifdef __cplusplus
extern "C" {
#endif

/* 读取csv文件时，解析每行情数据的各个字段的索引
 */
struct zdatafeed_bar_index_s
{
    int32_t     IndexInstrument;
    int32_t     IndexOpenPrice;
    int32_t     IndexHighPrice;
    int32_t     IndexLowPrice;
    int32_t     IndexClosePrice;
    int32_t     IndexSettlePrice;
    int32_t     IndexOpenInterest;
    int32_t     IndexAdjustFactor;
    int32_t     IndexVolume;
    int32_t     IndexTurnover;
    int32_t     IndexDateTime;
};
typedef struct zdatafeed_bar_index_s zdatafeed_bar_index_t;

/* 时间与该时间对应的所有产品的行情map
 */
struct zdatafeed_time2data_s
{
    int64_t     DataTime;        // the time of the bar data
    ztl_map_t*  AssetToData;     // <asset_hashid, zdatabar*>
};
typedef struct zdatafeed_time2data_s zdatafeed_time2data_t;

/* 一个产品的所有时间序列的行情
 */
struct zdatafeed_dataarr_s
{
    char        Symbol[32];
    ztl_array_t DataArray;
};
typedef struct zdatafeed_dataarr_s zdatafeed_dataarr_t;


extern int zs_data_load_csv(const char* filename, ztl_array_t* rawDatas);


#ifdef __cplusplus
}
#endif

#endif//_ZS_DATA_FEED_H_


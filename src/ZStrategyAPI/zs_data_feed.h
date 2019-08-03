/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
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

/* 字段类型
 */
typedef enum
{
    ZS_FT_Unknown       = 0,        // 代码
    ZS_FT_Symbol        = 1,        // 交易所
    ZS_FT_Exchange      = 2,        // 最高价
    ZS_FT_Last          = 3,        // 最新价
    ZS_FT_Open          = 4,        // 开盘价
    ZS_FT_High          = 5,        // 最高价
    ZS_FT_Low           = 6,        // 最低价
    ZS_FT_Close         = 7,        // 收盘价
    ZS_FT_Volume        = 8,        // 成交量
    ZS_FT_Amount        = 9,        // 成交额
    ZS_FT_OpenInterest  = 10,       // 持仓量
    ZS_FT_Settlement    = 11,       // 结算价
    ZS_FT_UpperLimit    = 12,       // 涨停价
    ZS_FT_LowerLimit    = 13,       // 跌停价
    ZS_FT_PreClose      = 14,       // 昨收盘
    ZS_FT_PreSettlement = 15,       // 昨结算
    ZS_FT_PreOpenInterest = 16,     // 昨持仓
    ZS_FT_Time          = 17,       // 更新时间
    ZS_FT_TradingDay    = 18,       // 交易日
    ZS_FT_ActionDay     = 19,       // 自然日
    ZS_FT_Reserve       = 20,       // 预留字段
    ZS_FT_BidPrice1     = 21,       // 买一价
    ZS_FT_BidPrice2     = 22,
    ZS_FT_BidPrice3     = 23,
    ZS_FT_BidPrice4     = 24,
    ZS_FT_BidPrice5     = 25,
    ZS_FT_BidPrice6     = 26,
    ZS_FT_BidPrice7     = 27,
    ZS_FT_BidPrice8     = 28,
    ZS_FT_BidPrice9     = 29,
    ZS_FT_BidPrice10    = 30,
    ZS_FT_BidVol1       = 31,       // 买一量
    ZS_FT_BidVol2       = 32,
    ZS_FT_BidVol3       = 33,
    ZS_FT_BidVol4       = 34,
    ZS_FT_BidVol5       = 35,
    ZS_FT_BidVol6       = 36,
    ZS_FT_BidVol7       = 37,
    ZS_FT_BidVol8       = 38,
    ZS_FT_BidVol9       = 39,
    ZS_FT_BidVol10      = 40,
    ZS_FT_AskPrice1     = 41,       // 卖一价
    ZS_FT_AskPrice2     = 42,
    ZS_FT_AskPrice3     = 43,
    ZS_FT_AskPrice4     = 44,
    ZS_FT_AskPrice5     = 45,
    ZS_FT_AskPrice6     = 46,
    ZS_FT_AskPrice7     = 47,
    ZS_FT_AskPrice8     = 48,
    ZS_FT_AskPrice9     = 49,
    ZS_FT_AskPrice10    = 50,
    ZS_FT_AskVol1       = 51,       // 卖一量
    ZS_FT_AskVol2       = 52,
    ZS_FT_AskVol3       = 53,
    ZS_FT_AskVol4       = 54,
    ZS_FT_AskVol5       = 55,
    ZS_FT_AskVol6       = 56,
    ZS_FT_AskVol7       = 57,
    ZS_FT_AskVol8       = 58,
    ZS_FT_AskVol9       = 59,
    ZS_FT_AskVol10      = 60,
    ZS_FT_PreDelta      = 19,
    ZS_FT_CurDelta      = 20,
    ZS_FT_Price         = 61,       // 价格
    ZS_FT_AdjustFactor  = 62,       // 复权因子
    ZS_FT_Underlying    = 63,       // 标的代码
    ZS_FT_Name          = 64,       // 名字
    ZS_FT_OptionType    = 65,       // 期权类型Call/Put
    ZS_FT_StikePrice    = 66,       // 执行价
    ZS_FT_TWAP          = 67,
    ZS_FT_VWAP          = 68
}ZSFieldType;


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


/* 字段名对应
 */
typedef struct zs_field_item_s
{
    const char* field_name;
    ZSFieldType field_type;
}zs_field_item_t;


/* csv loader，一个高性能，可扩展性的方便的csv loader
 * 1. 应该尽量做得开放
 * 2. 可以根据配置好的字段名映射，进行解析
 * 3. 或者根据指定的各字段的顺序，进行解析
 * 4. 数据内容解析由回调给上层应用进行，数据对象可以上层使用者分配
 * 5. 加载的内容，可提供类似 iterator 的接口，返回各行内容，顺带提供一个各行的str_delemeter接口
 * 6. csv loader 作为一个子应用模块，供其它模块使用，如果解析的数据，可解析到zs_series_t对象中
 * 
 */
typedef struct zs_csv_loader_s zs_csv_loader_t;
struct zs_csv_loader_s
{
    int     have_header;        // 是否有头部
    int     is_tick_data;       // 是否为tick数据
    int     us_shm;             // 是否使用共享内存
    char*   sep;                // csv字段分隔符
    void*   userdata1;          // 用户数据
    void*   userdata2;          // 用户数据
    const char* filename;       // 文件名

    // 解析列索引，预先按规定指定了列中各字段的含义，如 "open-->>OpenPx,high-->>HighPx"，
    // 其中前者为本文档规定的字段名值，后者为文件内容中的字段名
    // 示例内容："open=OpenPrice,high=HighPrice,volume=Volume"，由该loader解析
    const char* field_names_map;

    // 由外部指定各字段位于文件中的第几列，要取指定价格时，把访问此table中的字段指明的第几列值
    int32_t index_table[128];

    // 由平台从文件中读取数据行，并分隔成数组，每行内容由上层函数解析各字段内容
    int (*parse_line_fields)(zs_csv_loader_t* csv_loader, int num, zditem_t fields[], int size);

    // 内存分配函数
    void* (*alloc_ptr)(zs_csv_loader_t* csv_loader, int size);

    // 数据回调函数
    int (*on_line)(zs_csv_loader_t* csv_loader, int num, char* line, int length);
    int (*on_tick)(zs_csv_loader_t* csv_loader, zs_tick_t* tick);
    int (*on_bar)(zs_csv_loader_t*  csv_loader, zs_bar_t* bar);
};

// load csv data
int zs_data_load_csv(zs_csv_loader_t* csv_loader);


#ifdef __cplusplus
}
#endif

#endif//_ZS_DATA_FEED_H_

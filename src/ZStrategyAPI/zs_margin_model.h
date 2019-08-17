/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define margin model
 */

#ifndef _ZS_MARGIN_MODEL_INCLUDED_
#define _ZS_MARGIN_MODEL_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_map.h>

#include "zs_api_object.h"

#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif


typedef enum
{
    ZS_MC_Equity = 0,               // 股票
    ZS_MC_Future = 1,               // 期货
    ZS_MC_OptionETF = 2,            // ETF期权
    ZS_MC_OptionStock = 3,          // 个股期权
    ZS_MC_OptionIndex = 4,          // 指数期货期权
    ZS_MC_OptionCommodity = 5       // 商品期货期权
}ZSMarginAssetType;


/* 保证金计算模型 */
typedef struct zs_margin_model_s zs_margin_model_t;

struct zs_margin_model_s
{
    ZSMarginAssetType   AssetType;              // set by user
    zs_contract_t*      UnderlyingContract;     // set by user

    /* 计算保证金 var is usually underlying_price */
    double(*calculate)(zs_margin_model_t* model, zs_contract_t* contract,
        ZSDirection direction, int32_t volume, double price, double var);
};

/* 初始化一个保证金模型 */
int zs_margin_model_init(zs_margin_model_t* margin_model, ZSMarginAssetType asset_type);


#ifdef __cplusplus
}
#endif

#endif//_ZS_MARGIN_MODEL_INCLUDED_

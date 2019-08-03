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

typedef struct zs_margin_model_s zs_margin_model_t;

struct zs_margin_model_s
{
    const char* Name;
    void* UserData;

    double (*calculate)(zs_margin_model_t* model,
        const zs_order_t* order, const zs_trade_t* trade);
};



#ifdef __cplusplus
}
#endif

#endif//_ZS_MARGIN_MODEL_INCLUDED_

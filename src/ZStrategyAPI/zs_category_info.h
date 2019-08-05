/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define the category info
 */

#ifndef _ZS_CATEGORY_INFO_H_
#define _ZS_CATEGORY_INFO_H_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>

#include "zs_commission.h"
#include "zs_core.h"
#include "zs_hashdict.h"


#ifdef __cplusplus
extern "C" {
#endif

struct zs_category_info_s
{
    char    Product[8];     // future, stock, option
    char    Code[8];
    char    Exchange[8];
    char    Name[16];
    double  PriceTick;
    int32_t Multiplier;
    int32_t Decimal;
    int32_t Months[12];
    char    LastTradingDay[8];
    double  OpenRatio;
    double  CloseRatio;
    double  CloseTodayRatio;
    ZSCommissionType    CommType;
};
typedef struct zs_category_info_s zs_category_info_t;

struct zs_category_s
{
    ztl_pool_t*     Pool;
    ztl_dict_t*     Dict;
    uint32_t        Count;
};
typedef struct zs_category_s zs_category_t;


/* init object
 */
int zs_category_init(zs_category_t* category_obj);

int zs_category_release(zs_category_t* category_obj);

/* load category infos by setting file
 */
int zs_category_load(zs_category_t* category_obj, const char* info_file);

/* find category info
 */
zs_category_info_t* zs_category_find(zs_category_t* category_obj, const char* symbol);

int32_t zs_category_last_trading_day(zs_category_t* category_obj, const char* symbol);


/* get variety code integer
 */
uint32_t zs_get_variety_int(const char* symbol);


#ifdef __cplusplus
}
#endif

#endif//_ZS_CATEGORY_INFO_H_

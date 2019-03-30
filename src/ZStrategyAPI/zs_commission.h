/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define commission model
 */

#ifndef _ZS_COMMISSION_H_INCLUDED_
#define _ZS_COMMISSION_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_map.h>

#include "zs_api_object.h"

#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct zs_commission_s zs_commission_t;
typedef struct zs_commission_model_s zs_commission_model_t;

typedef struct zs_comm_per_share_s zs_comm_per_share_t;
typedef struct zs_comm_per_contract_s zs_comm_per_contract_t;

typedef enum
{
    ZS_COMMT_ByVolume,
    ZS_COMMT_ByMoney
}ZSCommissionType;

struct zs_commission_default_s
{
    const char* Exchange;
    const char* Name;
    double  MarginRatio;
    double  PriceTick;
    int     Multiplier;
    ZSCommissionType CommissionType;
    int     TickDecimal;
};

struct zs_comm_per_share_s
{
    double  BuyCost;
    double  SellCost;
    double  MinCost;
};

struct zs_comm_per_contract_s
{
    double  OpenCost;
    double  CloseCost;
    double  ClostTodayCost;
    float   Multiplier;
};

struct zs_commission_model_s
{
    const char* Name;
    void* UserData;

    double (*calculate)(zs_commission_model_t* model, 
        const zs_order_t* order, const zs_trade_t* trade);
};

struct zs_commission_s
{
    zs_algorithm_t*         Algorithm;
    zs_commission_model_t*  EquityModel;
    zs_commission_model_t*  FutureModel;

    zs_comm_per_share_t     PerShareCost;
    ztl_map_t*              VarietyMap;     // <variety, per_contract>
};


zs_commission_t* zs_commission_create(zs_algorithm_t* algo);

void zs_commission_release(zs_commission_t* comm);

void zs_commssion_set_per_share(zs_commission_t* comm, zs_comm_per_share_t* perShareCost);
void zs_commssion_set_per_contract(zs_commission_t* comm, zs_comm_per_contract_t* perContractCost, const char* symbol);

zs_commission_model_t* zs_commission_model_get(zs_commission_t* comm, int isEquity);

float zs_commission_calculate(zs_commission_t* comm, int isEquity, zs_order_t* order, zs_trade_t* trade);


#ifdef __cplusplus
}
#endif

#endif//_ZS_COMMISSION_H_INCLUDED_

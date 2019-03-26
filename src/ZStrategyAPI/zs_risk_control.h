/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the risk control 
 */

#ifndef _ZS_RISK_CONTROL_H_
#define _ZS_RISK_CONTROL_H_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_dlist.h>

#include "zs_api_object.h"

#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif

/* 
 * ·ç¿Ø¹ÜÀí
 */
struct zs_risk_control_s
{
    zs_algorithm_t* Algorithm;
    int32_t         MaxShares;
    float           MaxPositionRate;
};


int zs_risk_control_create(zs_risk_control_t* zrc);

void zs_risk_control_release(zs_risk_control_t* zrc);

int zs_risk_control_check(zs_risk_control_t* zrc, const zs_order_req_t* orderReq);



#ifdef __cplusplus
}
#endif

#endif//_ZS_RISK_CONTROL_H_

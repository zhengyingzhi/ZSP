/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the order struct 
 */

#ifndef _ZS_ORDER_H_INCLUDED_
#define _ZS_ORDER_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_dlist.h>

#include "zs_api_object.h"
#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif



typedef ztl_dlist_t zs_orderlist_t;

zs_orderlist_t* zs_orderlist_create();
void zs_orderlist_release(zs_orderlist_t* orderList);
int zs_orderlist_append(zs_orderlist_t* orderList, zs_order_t* pOrder);
int zs_orderlist_remove(zs_orderlist_t* orderList, zs_order_t* pOrder);

zs_order_t* zs_order_find(zs_orderlist_t* orderList, ZSExchangeID exchangeID,
    const char OrderSysID[]);


#ifdef __cplusplus
}
#endif

#endif//_ZS_ORDER_H_INCLUDED_

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
void zs_orderlist_release(zs_orderlist_t* orderlist);

int zs_orderlist_append(zs_orderlist_t* orderlist, zs_order_t* order);
int zs_orderlist_remove(zs_orderlist_t* orderlist, zs_order_t* order);

int zs_orderlist_size(zs_orderlist_t* orderlist);

int zs_orderlist_retrieve(zs_orderlist_t* orderlist, zs_order_t* orders[], int size);

zs_order_t* zs_order_find(zs_orderlist_t* orderlist, int32_t frontid, int32_t sessionid, 
    const char orderid[]);

zs_order_t* zs_order_find_by_sysid(zs_orderlist_t* orderlist, ZSExchangeID exchangeid,
    const char order_sysid[]);

#ifdef __cplusplus
}
#endif

#endif//_ZS_ORDER_H_INCLUDED_

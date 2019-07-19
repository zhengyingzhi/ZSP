/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the order container struct 
 */

#ifndef _ZS_ORDER_H_INCLUDED_
#define _ZS_ORDER_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_dlist.h>

#include "zs_api_object.h"
#include "zs_core.h"
#include "zs_hashdict.h"


#ifdef __cplusplus
extern "C" {
#endif


/* ¶©µ¥list
 */
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


//////////////////////////////////////////////////////////////////////////
/* ¶©µ¥dict <order_key, order>, without value dup
 */
typedef dict zs_orderdict_t;

typedef struct
{
    int32_t     SessionID;
    uint16_t    FrontID;
    uint16_t    Length;
    char*       pOrderID;
}ZSOrderKey;

typedef struct
{
    uint16_t    ExchangeID;
    uint16_t    Length;
    char*       pOrderSysID;
}ZSOrderSysKey;


zs_orderdict_t* zs_orderdict_create(ztl_pool_t* privptr);

void zs_orderdict_release(zs_orderdict_t* orderdict);

int zs_orderdict_add_order(zs_orderdict_t* orderdict, zs_order_t* value);

int zs_orderdict_add(zs_orderdict_t* orderdict, int32_t frontid, int32_t sessionid,
    const char orderid[], void* value);

int zs_orderdict_del(zs_orderdict_t* orderdict, int32_t frontid, int32_t sessionid,
    const char orderid[]);

int zs_orderdict_size(zs_orderdict_t* orderdict);

int zs_orderdict_retrieve(zs_orderdict_t* orderdict, zs_order_t* orders[], int size);

void* zs_orderdict_find(zs_orderdict_t* orderdict, int32_t frontid, int32_t sessionid,
    const char orderid[]);

void* zs_orderdict_find_by_sysid(zs_orderdict_t* orderdict, ZSExchangeID exchangeid,
    const char order_sysid[]);



#ifdef __cplusplus
}
#endif

#endif//_ZS_ORDER_H_INCLUDED_

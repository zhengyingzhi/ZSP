/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the algorithm api for strategy developer
 */

#ifndef _ZS_ALGORITHM_API_H_
#define _ZS_ALGORITHM_API_H_

#include <stdint.h>

#include "zs_assets.h"
#include "zs_core.h"

#ifdef __cplusplus
extern "C" {
#endif


/* place simple order:
 * quanty: >0 is buy, <0 is sell
 * price: ==0 is market order, otherwise limit order
 * stopOrder: is stop order or not
 * apiName: use what broker api to place the orde
 */
int zs_order(zs_algorithm_t* algo, const char* accountID, 
    zs_sid_t sid, int quanty, float limitPrice, int stopOrder, 
    const char* apiName);

int zs_order_ex(zs_algorithm_t* algo, zs_order_req_t* orderReq, int stopOrder, 
    const char* apiName);

int zs_order_target(zs_algorithm_t* algo, const char* accountID, 
    zs_sid_t sid, int targetQuanity, const char* apiName);

int zs_order_target_value(zs_algorithm_t* algo, const char* accountID, 
    zs_sid_t sid, float limitPrice, double targetValue, const char* apiName);

int zs_order_target_percent(zs_algorithm_t* algo, const char* accountID, 
    zs_sid_t sid, float limitPrice, float targetPercent, const char* apiName);

int zs_order_cancel(zs_algorithm_t* algo, int64_t orderId);

int zs_order_cancel_byorder(zs_algorithm_t* algo, zs_order_t* order);

int zs_order_cancel_batch(zs_algorithm_t* algo, const char* accountID, 
    const char* symbol, int directionFlag);

int zs_subscribe(zs_algorithm_t* algo, const char* symbol, const char* exchange);

int zs_get_open_orders(zs_algorithm_t* algo, const char* accountID, 
    const char* symbol, zs_order_t* orders[], int ordsize);


int zs_set_commission(zs_algorithm_t* algo);

int zs_set_margin(zs_algorithm_t* algo);

int zs_set_long_only(zs_algorithm_t* algo);

int zs_set_max_leverage(zs_algorithm_t* algo, float leverage);

zs_contract_t* zs_get_contract(zs_algorithm_t* algo, const char* symbol);

//zs_account_t* zs_get_account(zs_algorithm_t* algo, const char* accountID);

zs_portfolio_t* zs_get_portfolio(zs_algorithm_t* algo, const char* accountID);

int zs_handle_splits(zs_algorithm_t* algo, const char* symbol, float adjustFactor);


#ifdef __cplusplus
}
#endif

#endif//_ZS_ALGORITHM_API_H_

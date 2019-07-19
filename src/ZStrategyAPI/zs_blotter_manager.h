/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define blotter manager 
 */

#ifndef _ZS_BLOTTER_MANAGER_H_
#define _ZS_BLOTTER_MANAGER_H_

#include <stdint.h>

#include "zs_blotter.h"


#ifdef __cplusplus
extern "C" {
#endif

/* 管理各个blotter
 */
struct zs_blotter_manager_s
{
    zs_algorithm_t*     Algorithm;
    ztl_dict_t*         BlotterDict;    // 交易账户zs_blotter_t：资金，投资组合，持仓，报单等
    ztl_array_t*        BlotterArray;   // 交易账户的数组，便于遍历，BlotterDict则便于查找
};

void zs_blotter_manager_init(zs_blotter_manager_t* manager, zs_algorithm_t* algo);

void zs_blotter_manager_release(zs_blotter_manager_t* manager);

void zs_blotter_manager_add(zs_blotter_manager_t* manager, zs_blotter_t* blotter);

zs_blotter_t* zs_blotter_manager_get(zs_blotter_manager_t* manager, const char* accountID);


#ifdef __cplusplus
}
#endif

#endif//_ZS_BLOTTER_MANAGER_H_

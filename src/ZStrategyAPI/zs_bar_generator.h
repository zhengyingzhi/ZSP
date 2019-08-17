/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define bar generator
 */

#ifndef _ZS_BAR_GENERATOR_H_
#define _ZS_BAR_GENERATOR_H_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_palloc.h>

#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif


/* bar callback function type */
typedef void (*handle_bar_pt)(void* udata, zs_bar_t* bar);


/* bar generator */
typedef struct zs_bar_generator_s zs_bar_generator_t;
struct zs_bar_generator_s
{
    char        Symbol[32];
    uint64_t    Sid;
    int32_t     IsFinished;
    int32_t     IsFinishedX;
    uint32_t    XMin;
    zs_bar_t    CurrentBar;
    zs_bar_t    CurrentBarX;
    zs_tick_t   LastTick;

    void*           UserData;
    handle_bar_pt   handle_bar;
    handle_bar_pt   handle_barx;

    ztl_array_t*    BarList;
};

zs_bar_generator_t* zs_bargen_create(ztl_pool_t* pool, uint64_t sid);
void zs_bargen_init(zs_bar_generator_t* bargen, uint64_t sid);

void zs_bargen_release(zs_bar_generator_t* bargen);

void zs_bargen_set_handle_bar(zs_bar_generator_t* bargen, void* userdata,
    handle_bar_pt handle_bar_func,
    handle_bar_pt handle_barx_func);

void zs_bargen_update_tick(zs_bar_generator_t* bargen, zs_tick_t* tick);
void zs_bargen_update_tickl2(zs_bar_generator_t* bargen, zs_tickl2_t* tickl2);
void zs_bargen_update_bar(zs_bar_generator_t* bargen, zs_bar_t* bar);

#ifdef __cplusplus
}
#endif

#endif//_ZS_BAR_GENERATOR_H_

/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define bar series
 */

#ifndef _ZS_BAR_SERIES_H_
#define _ZS_BAR_SERIES_H_

#include <stdint.h>

#include <ZToolLib/ztl_vector.h>
#include <ZToolLib/ztl_palloc.h>

#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif

/* bar generator */
typedef struct zs_bar_series_s zs_bar_series_t;
struct zs_bar_series_s
{
    uint64_t        Sid;
    uint32_t        InitSize;
    uint32_t        Count;
    uint32_t        StartIndex;
    ztl_vector_t    OpenArray;
    ztl_vector_t    HighArray;
    ztl_vector_t    LowArray;
    ztl_vector_t    CloseArray;
};

zs_bar_series_t* zs_bar_series_create(ztl_pool_t* pool, uint32_t init_size);
void zs_bar_series_init(zs_bar_series_t* bar_series, uint32_t init_size);

void zs_bar_series_release(zs_bar_series_t* bar_series);

void zs_bar_series_push(zs_bar_series_t* bar_series, zs_bar_t* bar);
void zs_bar_series_push_batch(zs_bar_series_t* bar_series, zs_bar_t* bar[], int size);


#ifdef __cplusplus
}
#endif

#endif//_ZS_BAR_SERIES_H_

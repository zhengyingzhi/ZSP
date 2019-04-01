
#include "zs_bar_series.h"


zs_bar_series_t* zs_bar_series_create(ztl_pool_t* pool)
{
    zs_bar_series_t* bar_series;
    bar_series = ztl_pcalloc(pool, sizeof(zs_bar_series_t));
    return bar_series;
}

void zs_bar_series_init(zs_bar_series_t* bar_series)
{
}

void zs_bar_series_release(zs_bar_series_t* bar_series)
{
}

void zs_bar_series_push(zs_bar_series_t* bar_series, zs_bar_t* bar)
{
}

void zs_bar_series_push_batch(zs_bar_series_t* bar_series, zs_bar_t* bar[], int size)
{
}


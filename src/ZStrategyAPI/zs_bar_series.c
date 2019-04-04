
#include "zs_bar_series.h"


zs_bar_series_t* zs_bar_series_create(ztl_pool_t* pool, uint32_t init_size)
{
    zs_bar_series_t* bar_series;
    bar_series = ztl_pcalloc(pool, sizeof(zs_bar_series_t));
    zs_bar_series_init(bar_series, init_size);
    return bar_series;
}

void zs_bar_series_init(zs_bar_series_t* bar_series, uint32_t init_size)
{
    bar_series->InitSize = init_size;

    ztl_vector_init(&bar_series->OpenArray, 256, sizeof(double));
    ztl_vector_init(&bar_series->HighArray, 256, sizeof(double));
    ztl_vector_init(&bar_series->LowArray, 256, sizeof(double));
    ztl_vector_init(&bar_series->CloseArray, 256, sizeof(double));
}

void zs_bar_series_release(zs_bar_series_t* bar_series)
{
}

void zs_bar_series_push(zs_bar_series_t* bar_series, zs_bar_t* bar)
{
    bar_series->OpenArray.push_double(&bar_series->OpenArray, bar->OpenPrice);
    bar_series->HighArray.push_double(&bar_series->HighArray, bar->HighPrice);
    bar_series->LowArray.push_double(&bar_series->LowArray, bar->LowPrice);
    bar_series->CloseArray.push_double(&bar_series->CloseArray, bar->ClosePrice);

    if (bar_series->Count > bar_series->InitSize) {
        bar_series->StartIndex += 1;
    }
    else {
        bar_series->Count += 1;
    }
}

void zs_bar_series_push_batch(zs_bar_series_t* bar_series, zs_bar_t* bar[], int size)
{
    for (int i = 0; i < size; ++i)
    {
        zs_bar_series_push(bar_series, bar[i]);
    }
}


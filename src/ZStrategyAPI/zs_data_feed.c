
#include <ZToolLib/ztl_array.h>

#include <ZToolLib/ztl_palloc.h>

#include "zs_api_object.h"

#include "zs_data_feed.h"

int zs_data_load_csv(const char* filename, ztl_array_t* rawDatas)
{
    zs_bar_t* bar;

    for (int i = 1; i <=3; ++i)
    {
        bar = (zs_bar_t*)ztl_pcalloc(rawDatas->pool, sizeof(zs_bar_t));
        strcpy(bar->Symbol, "000001.SZA");
        bar->OpenPrice = 100.0f + i;
        bar->HighPrice = 100.0f + i + 5 + (i % 2);
        bar->LowPrice = 100.0f + i -4 - (i % 2);
        bar->ClosePrice = 100.0f + i + 2;

        bar->Volume = (100 / ((i % 2) + 1)) * 12500;
        bar->AdjustFactor = 1.0;

        bar->BarTime = 20180800 + i;
        bar->BarTime *= 1000000;

        zs_bar_t** ppaddr = (zs_bar_t**)ztl_array_push(rawDatas);
        *ppaddr = bar;
    }

    return 0;
}




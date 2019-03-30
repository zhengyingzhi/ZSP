
#include "zs_data_portal.h"


extern dictType sidHashDictType;


zs_data_portal_t* zs_data_portal_create()
{
    ztl_pool_t* pool;
    pool = ztl_create_pool(4096);

    zs_data_portal_t* data_portal;
    data_portal = (zs_data_portal_t*)ztl_pcalloc(pool, sizeof(zs_data_portal_t));

    data_portal->StartTime = 20180801000000;
    data_portal->EndTime = 20180803000000;
    data_portal->Pool = pool;
    data_portal->Time2Data = dictCreate(&sidHashDictType, data_portal);
    data_portal->Asset2Data = dictCreate(&sidHashDictType, data_portal);

    return data_portal;
}

void zs_data_portal_release(zs_data_portal_t* dataPortal)
{
    if (dataPortal->Time2Data) {
        dictRelease(dataPortal->Time2Data);
    }
    if (dataPortal->Asset2Data) {
        dictRelease(dataPortal->Asset2Data);
    }
}

int zs_data_portal_wrapper(zs_data_portal_t* dataPortal, ztl_array_t* rawDatas)
{
    dataPortal->RawDatas = rawDatas;

    zs_bar_t* bar = NULL;
    for (uint32_t i = 0; i < ztl_array_size(rawDatas); ++i)
    {
        // 实际上value应该是一个set/dict
        zs_bar_t* bar;
        bar = *(zs_bar_t**)ztl_array_at(rawDatas, i);
        dictAdd(dataPortal->Time2Data, (void*)bar->BarTime, (void*)bar);
    }

    if (bar)
    {
        // how to get sid here??
        int64_t sid = 1;
        dictAdd(dataPortal->Asset2Data, (void*)sid, rawDatas);
    }

    return 0;
}


zs_bar_t* zs_data_portal_get_bar(zs_data_portal_t* dataPortal, 
    zs_sid_t sid, int64_t dt)
{
    //if (sid < ZS_ASSET_START_SID) {
    //    return NULL;
    //}

    zs_bar_t* bar;
    bar = NULL;

    dictEntry* entry;
    entry = dictFind(dataPortal->Time2Data, (void*)sid);
    if (entry)
    {
        bar = (zs_bar_t*)entry->v.val;
    }

    return bar;
}

zs_bar_reader_t* zs_data_portal_get_barreader(zs_data_portal_t* dataPortal, int64_t dt)
{
    // data_portal could hold a bar_reader，取指定时间的所有产品的数据
    zs_bar_reader_t* bar_reader;
    bar_reader = NULL;

    return bar_reader;
}

int zs_data_portal_get3(zs_data_portal_t* dataPortal, ztl_array_t* dstArr, zs_sid_t sid, int64_t startdt, int64_t enddt)
{
    // sid==0 则取所有在该时间段的数据
    // sid!=0 则取指定产品在该时间段的数据
    return 0;
}


//////////////////////////////////////////////////////////////////////////
bool _zs_bar_reader_can_trade(zs_bar_reader_t* barReader, zs_sid_t sid)
{
    const zs_bar_t* bar;
    bar = zs_data_portal_get_bar(barReader->DataPortal, sid, barReader->CurrentDt);
    if (bar)
    {
        if (bar->Volume > 0 && bar->HighPrice > bar->LowPrice)
        {
            return true;
        }
    }
    return false;
}

int _zs_bar_reader_history(zs_bar_reader_t* barReader, zs_sid_t sid, zs_bar_t* barArr[], int arrSize)
{
    return 0;
}

double _zs_bar_reader_current(zs_bar_reader_t* barReader, zs_sid_t sid, const char* priceField)
{
    zs_bar_t* bar;
    bar = zs_data_portal_get_bar(barReader->DataPortal, sid, barReader->CurrentDt);
    if (!bar)
    {
        return 0;
    }

    if (strcmp(priceField, "close") == 0) {
        return bar->ClosePrice;
    }
    else if (strcmp(priceField, "open") == 0) {
        return bar->OpenPrice;
    }
    else if (strcmp(priceField, "high") == 0) {
        return bar->HighPrice;
    }
    else if (strcmp(priceField, "low") == 0) {
        return bar->LowPrice;
    }

    return 0;
}

zs_bar_t* _zs_bar_reader_current_bar(zs_bar_reader_t* barReader, zs_sid_t sid)
{
    return zs_data_portal_get_bar(barReader->DataPortal, sid, barReader->CurrentDt);
}

int zs_bar_reader_init(zs_bar_reader_t* barReader, zs_data_portal_t* dataPortal)
{
    barReader->DataPortal = dataPortal;
    barReader->DataFrequency = 0;
    barReader->CurrentDt = 0;

    barReader->can_trade = _zs_bar_reader_can_trade;
    barReader->history = _zs_bar_reader_history;
    barReader->current = _zs_bar_reader_current;
    barReader->current_bar = _zs_bar_reader_current_bar;

    return 0;
}


#include "zs_data_portal.h"


extern dictType uintHashDictType;


zs_data_portal_t* zs_data_portal_create()
{
    ztl_pool_t* pool;
    pool = ztl_create_pool(4096);

    zs_data_portal_t* data_portal;
    data_portal = (zs_data_portal_t*)ztl_pcalloc(pool, sizeof(zs_data_portal_t));

    data_portal->StartTime = 20180801000000;
    data_portal->EndTime = 20180803000000;
    data_portal->Pool = pool;
    data_portal->Time2Data = dictCreate(&uintHashDictType, data_portal);
    data_portal->Asset2Data = dictCreate(&uintHashDictType, data_portal);

    return data_portal;
}

void zs_data_portal_release(zs_data_portal_t* data_portal)
{
    if (data_portal->Time2Data) {
        dictRelease(data_portal->Time2Data);
    }
    if (data_portal->Asset2Data) {
        dictRelease(data_portal->Asset2Data);
    }
}


int zs_data_portal_wrapper(zs_data_portal_t* data_portal, ztl_array_t* raw_datas)
{
    data_portal->RawDatas = raw_datas;

    zs_bar_t* bar = NULL;
    for (uint32_t i = 0; i < ztl_array_size(raw_datas); ++i)
    {
        // 实际上value应该是一个set/dict
        zs_bar_t* bar;
        bar = (zs_bar_t*)ztl_array_at2(raw_datas, i);
        dictAdd(data_portal->Time2Data, (void*)bar->BarTime, (void*)bar);
    }

    if (bar)
    {
        // how to get sid here??
        int64_t sid = 1;
        dictAdd(data_portal->Asset2Data, (void*)sid, raw_datas);
    }

    return 0;
}


zs_bar_t* zs_data_portal_get_bar(zs_data_portal_t* data_portal, 
    zs_sid_t sid, int64_t dt)
{
    //if (sid < ZS_ASSET_START_SID) {
    //    return NULL;
    //}

    zs_bar_t* bar;
    bar = NULL;

    dictEntry* entry;
    entry = dictFind(data_portal->Time2Data, (void*)sid);
    if (entry)
    {
        bar = (zs_bar_t*)entry->v.val;
    }

    return bar;
}

zs_bar_reader_t* zs_data_portal_get_barreader(zs_data_portal_t* data_portal, int64_t dt)
{
    // data_portal could hold a bar_reader，取指定时间的所有产品的数据
    zs_bar_reader_t* bar_reader;
    bar_reader = NULL;

    return bar_reader;
}

int zs_data_portal_get3(zs_data_portal_t* data_portal, ztl_array_t* dstarr, zs_sid_t sid, int64_t startdt, int64_t enddt)
{
    // sid==0 则取所有在该时间段的数据
    // sid!=0 则取指定产品在该时间段的数据
    return 0;
}


//////////////////////////////////////////////////////////////////////////
bool _zs_bar_reader_can_trade(zs_bar_reader_t* bar_reader, zs_sid_t sid)
{
    const zs_bar_t* bar;
    bar = zs_data_portal_get_bar(bar_reader->DataPortal, sid, bar_reader->CurrentDt);
    if (bar)
    {
        if (bar->Volume > 0 && bar->HighPrice > bar->LowPrice)
        {
            return true;
        }
    }
    return false;
}


int _zs_bar_reader_history(zs_bar_reader_t* bar_reader, zs_sid_t sid, zs_bar_t* arr[], int size)
{
    return 0;
}

double _zs_bar_reader_current(zs_bar_reader_t* bar_reader, zs_sid_t sid, const char* field)
{
    zs_bar_t* bar;

    if (bar_reader->DataPortal)
        bar = zs_data_portal_get_bar(bar_reader->DataPortal, sid, bar_reader->CurrentDt);
    else
        bar = &bar_reader->Bar;

    if (!bar) {
        return 0;
    }

    if (strcmp(field, "price") == 0) {
        return bar->ClosePrice;
    }
    else if (strcmp(field, "close") == 0) {
        return bar->ClosePrice;
    }
    else if (strcmp(field, "open") == 0) {
        return bar->OpenPrice;
    }
    else if (strcmp(field, "high") == 0) {
        return bar->HighPrice;
    }
    else if (strcmp(field, "low") == 0) {
        return bar->LowPrice;
    }
    else if (strcmp(field, "volume") == 0) {
        return (double)bar->Volume;
    }
    else if (strcmp(field, "amount") == 0) {
        return bar->Amount;
    }
    else if (strcmp(field, "adjust_factor") == 0) {
        return bar->AdjustFactor;
    }

    return 0;
}


double _zs_bar_reader_current2(zs_bar_reader_t* bar_reader, zs_sid_t sid, ZSFieldType field)
{
    zs_bar_t* bar;

    if (bar_reader->DataPortal)
        bar = zs_data_portal_get_bar(bar_reader->DataPortal, sid, bar_reader->CurrentDt);
    else
        bar = &bar_reader->Bar;

    if (!bar) {
        return 0;
    }

    switch (field)
    {
    case ZS_FT_Price:
    case ZS_FT_Close:
        return bar->ClosePrice;
    case ZS_FT_Open:
        return bar->OpenPrice;
    case ZS_FT_High:
        return bar->HighPrice;
    case ZS_FT_Low:
        return bar->LowPrice;
    case ZS_FT_Volume:
        return (double)bar->Volume;
    case ZS_FT_Amount:
        return bar->Amount;
    case ZS_FT_OpenInterest:
        return bar->OpenInterest;
    case ZS_FT_Settlement:
        return bar->SettlePrice;
    case ZS_FT_AdjustFactor:
        return bar->AdjustFactor;
    case ZS_FT_Time:
        return (double)bar->BarTime;
    default:
        return 0;
    }
}

zs_bar_t* _zs_bar_reader_current_bar(zs_bar_reader_t* bar_reader, zs_sid_t sid)
{
    return zs_data_portal_get_bar(bar_reader->DataPortal, sid, bar_reader->CurrentDt);
}

int zs_bar_reader_init(zs_bar_reader_t* bar_reader, zs_data_portal_t* data_portal)
{
    bar_reader->DataPortal = data_portal;
    bar_reader->DataFrequency = 0;
    bar_reader->CurrentDt = 0;

    bar_reader->can_trade = _zs_bar_reader_can_trade;
    bar_reader->history = _zs_bar_reader_history;
    bar_reader->current = _zs_bar_reader_current;
    bar_reader->current2 = _zs_bar_reader_current2;
    bar_reader->current_bar = _zs_bar_reader_current_bar;

    return 0;
}


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

void zs_data_portal_release(zs_data_portal_t* data_portal)
{
    if (data_portal->Time2Data) {
        dictRelease(data_portal->Time2Data);
    }
    if (data_portal->Asset2Data) {
        dictRelease(data_portal->Asset2Data);
    }
}

int zs_data_portal_wrapper(zs_data_portal_t* data_portal, ztl_array_t* rawDatas)
{
    data_portal->RawDatas = rawDatas;

    zs_bar_t* bar = NULL;
    for (uint32_t i = 0; i < ztl_array_size(rawDatas); ++i)
    {
        // ʵ����valueӦ����һ��set/dict
        zs_bar_t* bar;
        bar = *(zs_bar_t**)ztl_array_at(rawDatas, i);
        dictAdd(data_portal->Time2Data, (void*)bar->BarTime, (void*)bar);
    }

    if (bar)
    {
        // how to get sid here??
        int64_t sid = 1;
        dictAdd(data_portal->Asset2Data, (void*)sid, rawDatas);
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
    // data_portal could hold a bar_reader��ȡָ��ʱ������в�Ʒ������
    zs_bar_reader_t* bar_reader;
    bar_reader = NULL;

    return bar_reader;
}

int zs_data_portal_get3(zs_data_portal_t* data_portal, ztl_array_t* dstarr, zs_sid_t sid, int64_t startdt, int64_t enddt)
{
    // sid==0 ��ȡ�����ڸ�ʱ��ε�����
    // sid!=0 ��ȡָ����Ʒ�ڸ�ʱ��ε�����
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

int _zs_bar_reader_history(zs_bar_reader_t* bar_reader, zs_sid_t sid, zs_bar_t* barArr[], int arrSize)
{
    return 0;
}

double _zs_bar_reader_current(zs_bar_reader_t* bar_reader, zs_sid_t sid, const char* field)
{
    zs_bar_t* bar;
    bar = zs_data_portal_get_bar(bar_reader->DataPortal, sid, bar_reader->CurrentDt);
    if (!bar)
    {
        return 0;
    }

    if (strcmp(field, "close") == 0) {
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

    return 0;
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
    bar_reader->current_bar = _zs_bar_reader_current_bar;

    return 0;
}

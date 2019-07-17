
#include <ZToolLib/ztl_array.h>

#include "zs_algorithm.h"

#include "zs_configs.h"

#include "zs_event_engine.h"


static bool _zs_pc_handler(ztl_producer_consumer_t* zpc, int64_t type, void* data);


zs_event_engine_t* zs_ee_create(ztl_pool_t* pool, ZSRunMode run_mode)
{
    zs_event_engine_t* ee;

    ee = (zs_event_engine_t*)ztl_pcalloc(pool, sizeof(zs_event_engine_t));

    ee->Pool = pool;

    if (run_mode == ZS_RM_Backtest)
    {
        ee->ZPCCore = NULL;
    }
    else
    {
        ee->ZPCCore = ztl_pc_create(ZS_EVENT_ENGINE_MAX_QUEUED);
        ztl_pc_set_udata(ee->ZPCCore, ee);
    }

    return ee;
}

void zs_ee_release(zs_event_engine_t* ee)
{
    if (ee)
    {
        if (ee->ZPCCore)
        {
            ztl_pc_release(ee->ZPCCore);
            ee->ZPCCore = NULL;
        }
    }
}

void zs_ee_start(zs_event_engine_t* ee)
{
    if (ee->ZPCCore)
        ztl_pc_start(ee->ZPCCore);
}

void zs_ee_stop(zs_event_engine_t* ee)
{
    if (ee->ZPCCore)
        ztl_pc_stop(ee->ZPCCore);
}

int zs_ee_register(zs_event_engine_t* ee, void* userdata, uint32_t evtype, zs_ee_handler_pt handler)
{
    if (evtype >= ZS_EVENT_ENGINE_MAX_EVID)
    {
        return -1;
    }

    ztl_array_t* arr;
    arr = ee->EvObjectTable[evtype];

    if (!arr)
    {
        arr = (ztl_array_t*)ztl_pcalloc(ee->Pool, sizeof(ztl_array_t));
        ztl_array_init(arr, ee->Pool, 8, sizeof(zs_evobject_t));
        ee->EvObjectTable[evtype] = arr;
    }

    zs_evobject_t* lpobj;
    lpobj = (zs_evobject_t*)ztl_array_push(arr);
    lpobj->EvUserData = userdata;
    lpobj->EvHandler = handler;

    return 0;
}

int zs_ee_post(zs_event_engine_t* ee, uint32_t evtype, void* evdata)
{
    int rv;
    if (ee->ZPCCore)
    {
        rv = ztl_pc_post(ee->ZPCCore, _zs_pc_handler, evtype, evdata);
    }
    else
    {
        rv = zs_ee_do_callback(ee, evtype, evdata);
    }
    return rv;
}

int zs_ee_do_callback(zs_event_engine_t* ee, uint32_t evtype, void* evdata)
{
    ztl_array_t* evobjArr;
    evobjArr = ee->EvObjectTable[evtype];
    if (!evobjArr)
    {
        // not regist before
        return -1;
    }

    for (uint32_t i = 0; i < ztl_array_size(evobjArr); ++i)
    {
        zs_evobject_t* evobj = (zs_evobject_t*)ztl_array_at(evobjArr, i);
        if (evobj->EvHandler)
            evobj->EvHandler(ee, evobj->EvUserData, evtype, evdata);
    }
    return 0;
}


static bool _zs_pc_handler(ztl_producer_consumer_t* zpc, int64_t type, void* data)
{
    uint32_t evtype;
    zs_data_head_t* zdh;
    zs_event_engine_t* ee;
    ee = (zs_event_engine_t*)ztl_pc_get_udata(zpc);

    zdh = (zs_data_head_t*)data;

    evtype = (uint32_t)type;

    /* 根据事件类型，回调事件处理函数 */
    // 回调数据是否是 zdh 还是 zd_data_body(zdh) ?
    //zs_ee_do_callback(ee, evtype, zd_data_body(zdh));
    zs_ee_do_callback(ee, evtype, zdh);

    zs_data_decre_release(zdh);

    return true;
}

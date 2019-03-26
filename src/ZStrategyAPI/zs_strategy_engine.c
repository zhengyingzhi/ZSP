
#include <ZToolLib/ztl_dyso.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_data_portal.h"

#include "zs_event_engine.h"

#include "zs_strategy_api.h"

#include "zs_strategy_engine.h"

// demo use
#include "zs_strategy_demo.h"




static uint32_t _zs_strategy_retrieve(zs_strategy_engine_t* zse, zs_sid_t sid,
    zs_strategy_api_t* stgArray[], uint32_t arrSize)
{
    uint32_t count = 0;
    if (sid == 0)
    {
        for (uint32_t i = 0; i < ztl_array_size(&zse->AllStrategy) && i < arrSize; ++i)
        {
            void* stg = ztl_array_at((&zse->AllStrategy), i);
            if (!stg) {
                continue;
            }
            stgArray[count++] = stg;
        }
    }
    else
    {
        stgArray[0] = ztl_map_find(zse->StrategyMap, sid);
        if (stgArray[0])
            count++;
    }
    return count;
}

/* zs_strategy event handlers */
static void _zs_strategy_handle_order(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    // 处理订单回报事件：
    // 根据type和data，过滤策略，然后回调
    zs_data_head_t*         zdh;
    zs_strategy_engine_t*   zse;
    zs_strategy_api_t*      stg;
    zs_strategy_api_t*      stg_array[16];
    zs_order_t*             order;
    zs_sid_t                sid;

    zse = ee->Algorithm->StrategyEngine;
    zdh = (zs_data_head_t*)evdata;
    order = (zs_order_t*)zd_data_body(zdh);

    //sid = zs_asset_lookup(strategies->AssetFinder, order->Symbol, strlen(order->Symbol));
    sid = zs_asset_lookup(zse->AssetFinder, zdh->pSymbol, zdh->SymbolLength);

    uint32_t count = _zs_strategy_retrieve(zse, sid, stg_array, 16);
    for (uint32_t i = 0; i < count; ++i)
    {
        stg = (zs_strategy_api_t*)stg_array[i];
        if (stg && stg->on_order)
            stg->on_order(stg->Instrance, zse->Algorithm, order);
    }
}

static void _zs_strategy_handle_trade(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    // 同理处理成交事件
    zs_data_head_t*         zdh;
    zs_strategy_engine_t*   zse;
    zs_strategy_api_t*      stg;
    zs_strategy_api_t*      stg_array[16];
    zs_trade_t*             trade;
    zs_sid_t                sid;

    zse = ee->Algorithm->StrategyEngine;
    zdh = (zs_data_head_t*)evdata;
    trade = (zs_trade_t*)zd_data_body(zdh);

    //sid = zs_asset_lookup(strategies->AssetFinder, order->Symbol, strlen(order->Symbol));
    sid = zs_asset_lookup(zse->AssetFinder, zdh->pSymbol, zdh->SymbolLength);

    uint32_t count = _zs_strategy_retrieve(zse, sid, stg_array, 16);
    for (uint32_t i = 0; i < count; ++i)
    {
        stg = (zs_strategy_api_t*)stg_array[i];
        if (stg && stg->on_trade)
            stg->on_trade(stg->Instrance, zse->Algorithm, trade);
    }
}

static void _zs_strategy_handle_md(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    // 处理行情事件(是否需要优化为多线程)
    zs_data_head_t*         zdh;
    zs_strategy_engine_t*   zse;
    zs_strategy_api_t*      stg;
    zs_strategy_api_t*      stg_array[16];
    void*                   data_body;
    zs_sid_t                sid;

    zse = ee->Algorithm->StrategyEngine;
    zdh = (zs_data_head_t*)evdata;
    data_body = zd_data_body(zdh);

    //sid = zs_asset_lookup(strategies->AssetFinder, order->Symbol, strlen(order->Symbol));
    if (zdh->DType != ZS_DT_MD_Bar)
        sid = zs_asset_lookup(zse->AssetFinder, zdh->pSymbol, zdh->SymbolLength);
    else
        sid = 16;

    uint32_t count = _zs_strategy_retrieve(zse, sid, stg_array, 16);
    for (uint32_t i = 0; i < count; ++i)
    {
        stg = (zs_strategy_api_t*)stg_array[i];
        if (!stg)
        {
            continue;
        }

        if (zdh->DType == ZS_DT_MD_Tick)
        {
            if (stg->on_tickdata)
                stg->on_tickdata(stg, zse->Algorithm, (zs_tick_t*)data_body);
        }
        else if (zdh->DType == ZS_DT_MD_Level2)
        {
            if (stg->on_tickdataL2)
                stg->on_tickdataL2(stg, zse->Algorithm, (zs_l2_tick_t*)data_body);
        }
        else if (zdh->DType == ZS_DT_MD_Bar)
        {
            // should make a default bar_reader, and just replace its data each time
            zs_bar_reader_t* bar_reader = NULL;
            memcpy(&bar_reader, data_body, sizeof(void*));

            if (stg->on_bardata)
                stg->on_bardata(stg->Instrance, zse->Algorithm, bar_reader);
        }
    }
}


/* zs_strategy engine */
zs_strategy_engine_t* zs_strategy_engine_create(zs_algorithm_t* algo)
{
    zs_strategy_engine_t* zse;

    zse = (zs_strategy_engine_t*)ztl_pcalloc(algo->Pool, sizeof(zs_strategy_engine_t));

    zse->Algorithm = algo;
    zse->AssetFinder = algo->AssetFinder;
    zse->Pool = algo->Pool;
    zse->StrategyMap = ztl_map_create(16);

    ztl_array_init(&zse->AllStrategy, algo->Pool, 16, sizeof(void*));

    return zse;
}

void zs_strategy_engine_release(zs_strategy_engine_t* zse)
{
    if (zse) {
        free(zse);
    }
}

static void zs_strategy_register_event(zs_strategy_engine_t* zse)
{
    zs_event_engine_t* ee = zse->Algorithm->EventEngine;
    zs_ee_register(ee, zse, ZS_EV_Order, _zs_strategy_handle_order);
    zs_ee_register(ee, zse, ZS_EV_Trade, _zs_strategy_handle_trade);
    zs_ee_register(ee, zse, ZS_EV_MD, _zs_strategy_handle_md);
}

int zs_strategy_engine_load(zs_strategy_engine_t* zse, ztl_array_t* stgLibPaths)
{
    zs_strategy_register_event(zse);

    // load all strategies from dso
    const char* lib_path;
    for (uint32_t i = 0; i < ztl_array_size(stgLibPaths); ++i)
    {
        lib_path = ztl_array_at(stgLibPaths, i);
        zs_strategy_load(zse, lib_path);
    }

    // load demo strategy
    zs_strategy_api_t* stgd = NULL;
    zs_strategy_entry(&stgd);
    stgd->Instrance = stgd->create("", 0);

    ztl_map_add(zse->StrategyMap, 16, stgd);
    void** dst = ztl_array_push(&zse->AllStrategy);
    *dst = stgd;

    return 0;
}

int zs_strategy_load(zs_strategy_engine_t* zse, const char* libPath)
{
    ztl_dso_handle_t* dso;
    dso = ztl_dso_load(libPath);
    if (!dso)
    {
        // log error
        return -1;
    }

    // TODO retrieve func from dso, and get its tdapi & mdapi
    // then, make some relationship

    zs_strategy_api_t* stg;
    stg = (zs_strategy_api_t*)ztl_pcalloc(zse->Pool, sizeof(zs_strategy_api_t));

    stg->HLib = dso;

    // add to map and array
    ztl_map_add(zse->StrategyMap, 1, stg);

    void** dst = ztl_array_push(&zse->AllStrategy);
    *dst = stg;

    return 0;
}

int zs_strategy_unload(zs_strategy_engine_t* zse, const char* stgName)
{
    return 0;
}

int zs_strategy_find(zs_strategy_engine_t* zse, uint32_t sid, zs_strategy_api_t* stgArray[])
{
    int index = 0;
    zs_strategy_api_t* stg;
    stg = ztl_map_find(zse->StrategyMap, sid);

    stgArray[index++] = stg;

    return index;
}



#include <ZToolLib/ztl_dyso.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_data_portal.h"

#include "zs_event_engine.h"

#include "zs_strategy_entry.h"

#include "zs_strategy_engine.h"

// demo use
#include "zs_strategy_demo.h"

typedef struct  
{
    char*   StrategyName;       // 策略名
    char*   StrategyPath;       // 策略路径
}zs_strategy_file_t;



static uint32_t _zs_strategy_retrieve(zs_strategy_engine_t* zse, zs_sid_t sid,
    zs_strategy_entry_t* stgArray[], uint32_t arrSize)
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
    zs_strategy_entry_t*    stg;
    zs_strategy_entry_t*    stg_array[16];
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
        stg = (zs_strategy_entry_t*)stg_array[i];
        if (stg && stg->handle_order)
            stg->handle_order(0, order);
    }
}

static void _zs_strategy_handle_trade(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    // 同理处理成交事件
    zs_data_head_t*         zdh;
    zs_strategy_engine_t*   zse;
    zs_strategy_entry_t*    stg;
    zs_strategy_entry_t*    stg_array[16];
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
        stg = (zs_strategy_entry_t*)stg_array[i];
        if (stg && stg->handle_trade)
            stg->handle_trade(0, trade);
    }
}

static void _zs_strategy_handle_md(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    // 处理行情事件(是否需要优化为多线程)
    zs_data_head_t*         zdh;
    zs_strategy_engine_t*   zse;
    zs_strategy_entry_t*    stg_entry;
    zs_strategy_entry_t*    stg_array[16];
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
        stg_entry = (zs_strategy_entry_t*)stg_array[i];
        if (!stg_entry)
        {
            continue;
        }

        if (zdh->DType == ZS_DT_MD_Tick)
        {
            if (stg_entry->handle_tickdata)
                stg_entry->handle_tickdata(NULL/*FIXME*/, (zs_tick_t*)data_body);
        }
        else if (zdh->DType == ZS_DT_MD_Level2)
        {
            if (stg_entry->handle_tickl2data)
                stg_entry->handle_tickl2data(NULL, (zs_tickl2_t*)data_body);
        }
        else if (zdh->DType == ZS_DT_MD_Bar)
        {
            // should make a default bar_reader, and just replace its data each time
            zs_bar_reader_t* bar_reader = NULL;
            memcpy(&bar_reader, data_body, sizeof(void*));

            if (stg_entry->handle_bardata)
                stg_entry->handle_bardata(NULL, bar_reader);
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

    // TODO: 需要预先从配置文件中读取当前系统支持哪些策略
    //       也可自动定义规则，扫描当前目录下strategy开头的文件且为dll/so形式的，然后自动加载
    zse->StrategyPaths = ztl_vector_create(32, sizeof(zs_strategy_file_t*));

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

int zs_strategy_engine_load(zs_strategy_engine_t* zse, ztl_array_t* stg_libpaths)
{
    zs_strategy_register_event(zse);

    // load all strategies from dso
    const char* lib_path;
    for (uint32_t i = 0; i < ztl_array_size(stg_libpaths); ++i)
    {
        lib_path = ztl_array_at(stg_libpaths, i);
        zs_strategy_load(zse, lib_path);
    }

    // load demo strategy
    zs_strategy_entry_t* stg_entry = NULL;
    zs_demo_strategy_entry(&stg_entry);
    if (!stg_entry) {
        // ERRORID: no entry callbacks
        return -1;
    }

    zs_cta_strategy_t* strategy;
    strategy = ztl_palloc(zse->Pool, sizeof(zs_cta_strategy_t));
    strategy->Instance = stg_entry->create("");

    ztl_map_add(zse->StrategyMap, 16, strategy);
    void** dst = ztl_array_push(&zse->AllStrategy);
    *dst = strategy;

    return 0;
}

int zs_strategy_load(zs_strategy_engine_t* zse, const char* libpath)
{
    ztl_dso_handle_t* dso;
    dso = ztl_dso_load(libpath);
    if (!dso)
    {
        // log error
        return -1;
    }

    // TODO retrieve func from dso, and get its tdapi & mdapi
    // then, make some relationship

    zs_cta_strategy_t* strategy;
    strategy = (zs_cta_strategy_t*)ztl_pcalloc(zse->Pool, sizeof(zs_cta_strategy_t));

    strategy->HLib = dso;

    strategy->StrategyID = zse->StrategyBaseID++;

    // add to map and array
    ztl_map_add(zse->StrategyMap, strategy->StrategyID, strategy);

    void** dst = ztl_array_push(&zse->AllStrategy);
    *dst = strategy;

    return 0;
}

int zs_strategy_unload(zs_strategy_engine_t* zse, const char* strategy_name)
{
    return 0;
}

int zs_strategy_find(zs_strategy_engine_t* zse, uint32_t strategy_id, zs_cta_strategy_t* stgArray[])
{
    int index = 0;
    zs_cta_strategy_t* stg;
    stg = ztl_map_find(zse->StrategyMap, strategy_id);

    stgArray[index++] = stg;

    return index;
}

int zs_strategy_add(zs_strategy_engine_t* zse, const char* setting)
{
    // 提供一个配置解析工具，该工具主要为可解析key-value形式的数据，value可以为list
    // 根据配置添加一个策略，根据名字，从StrategyPaths中查找策略路径，
    // 并加载策略动态库，并得到zs_strategy_api_t*，即策略的入口函数等
    // 生成一个zs_cta_strategy_t类型，保存该策略的基本参数，账户，策略入口函数等
    // 将该cta_strategy实例添加到engine的字典关系中，便于访问
    return 0;
}

int zs_strategy_del(zs_strategy_engine_t* zse, uint32_t strategy_id)
{
    // 移除策略
    return 0;
}

int zs_strategy_start(zs_strategy_engine_t* zse, uint32_t strategy_id)
{
    return 0;
}

int zs_strategy_stop(zs_strategy_engine_t* zse, uint32_t strategy_id)
{
    return 0;
}

int zs_strategy_update(zs_strategy_engine_t* zse, uint32_t strategy_id, const char* setting)
{
    return 0;
}


void zs_strategy_on_order_req(zs_strategy_engine_t* zse, zs_order_req_t* order_req, uint64_t* order_id)
{
    // called by zs_cta_strategy when order placed, and got the order_id
    // here, we save the order relationship for order returned
}

void zs_strategy_on_order_rtn(zs_strategy_engine_t* zse, zs_order_t* order)
{
    // process the order returned event, call its strategy's callback
}

void zs_strategy_on_trade_rtn(zs_strategy_engine_t* zse, zs_trade_t* trade)
{
}

void zs_strategy_on_tick(zs_strategy_engine_t* zse, zs_tick_t* tick)
{
}

void zs_strategy_on_tickl2(zs_strategy_engine_t* zse, zs_tickl2_t* tickl2)
{
}



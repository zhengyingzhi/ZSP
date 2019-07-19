
#include <ZToolLib/ztl_dyso.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_data_portal.h"

#include "zs_event_engine.h"

#include "zs_strategy_entry.h"

#include "zs_strategy_engine.h"

// demo use
#include "zs_strategy_demo.h"


// 默认最大策略数
#define MAX_STRATEGY_COUNT  32



static int _cta_strategy_object_comp(void* expect, void* actual)
{
    return expect == actual;
}

static int _cta_strategy_name_comp(void* expect, void* actual)
{
    char* name = (char*)expect;
    zs_cta_strategy_t* s = (zs_cta_strategy_t*)actual;
    return strcmp(s->Entry->StrategyName, name) == 0;
}

static int _strategy_entry_name_comp(void* expect, void* actual)
{
    char* name = (char*)expect;
    zs_strategy_entry_t* s = (zs_strategy_entry_t*)actual;
    return strcmp(s->StrategyName, name) == 0;
}

static void zs_strategy_register_event(zs_strategy_engine_t* zse);

static uint32_t _zs_strategy_retrieve(zs_strategy_engine_t* zse, zs_sid_t sid,
    zs_cta_strategy_t* strategy_array[], uint32_t array_size)
{
    uint32_t count = 0;
    if (sid == 0)
    {
        for (uint32_t i = 0; i < ztl_array_size(zse->AllStrategy) && i < array_size; ++i)
        {
            void* strategy = ztl_array_at(zse->AllStrategy, i);
            if (!strategy) {
                continue;
            }
            strategy_array[count++] = strategy;
        }
    }
    else
    {
        strategy_array[0] = ztl_map_find(zse->StrategyMap, sid);
        if (strategy_array[0])
            count++;
    }
    return count;
}

/* zs_strategy event handlers */
static void _zs_strategy_handle_order(zs_event_engine_t* ee, void* userdata,
    uint32_t evtype, void* evdata)
{
    // 处理订单回报事件
    zs_strategy_engine_t*   zse;
    zs_cta_strategy_t*      strategy;
    zs_cta_strategy_t*      strategy_array[MAX_STRATEGY_COUNT];
    zs_data_head_t*         zdh;
    zs_order_t*             order;
    zs_sid_t                sid;
    uint32_t                count;

    zse = (zs_strategy_engine_t*)userdata;
    zdh = (zs_data_head_t*)evdata;
    order = (zs_order_t*)zd_data_body(zdh);
    sid = order->Sid;

    // find the strategy related the order
    strategy = zs_orderdict_find(zse->OrderStrategyDict, order->FrontID, order->SessionID, order->OrderID);
    if (strategy && strategy->Entry->handle_order)
        strategy->Entry->handle_order(strategy->Instance, strategy, order);
}

static void _zs_strategy_handle_trade(zs_event_engine_t* ee, void* userdata,
    uint32_t evtype, void* evdata)
{
    // 同理处理成交事件
    zs_strategy_engine_t*   zse;
    zs_cta_strategy_t*      strategy;
    //zs_cta_strategy_t*      strategy_array[MAX_STRATEGY_COUNT];
    zs_data_head_t*         zdh;
    zs_trade_t*             trade;
    zs_sid_t                sid;

    zse = (zs_strategy_engine_t*)userdata;
    zdh = (zs_data_head_t*)evdata;
    trade = (zs_trade_t*)zd_data_body(zdh);
    sid = trade->Sid;

#if 0
    uint32_t count = _zs_strategy_retrieve(zse, sid, strategy_array, MAX_STRATEGY_COUNT);
    for (uint32_t i = 0; i < count; ++i)
    {
        strategy = (zs_cta_strategy_t*)strategy_array[i];
        if (!strategy)
            continue;

        if (strategy->Entry->handle_trade)
            strategy->Entry->handle_trade(strategy->Instance, strategy, trade);
    }
#endif//0
}

static void _zs_strategy_handle_tick(zs_event_engine_t* ee, void* userdata, 
    uint32_t evtype, void* evdata)
{
    // 处理行情事件(是否需要优化为多线程)
    zs_strategy_engine_t*   zse;
    zs_cta_strategy_t*      strategy;
    zs_cta_strategy_t*      strategy_array[MAX_STRATEGY_COUNT];
    zs_data_head_t*         zdh;
    zs_tick_t*              tick;
    zs_sid_t                sid;
    uint32_t                count;

    zse = (zs_strategy_engine_t*)userdata;
    zdh = (zs_data_head_t*)evdata;
    tick = (zs_tick_t*)zd_data_body(zdh);
    sid = tick->Sid;

    count = _zs_strategy_retrieve(zse, sid, strategy_array, MAX_STRATEGY_COUNT);
    for (uint32_t i = 0; i < count; ++i)
    {
        strategy = (zs_cta_strategy_t*)strategy_array[i];
        if (!strategy)
        {
            continue;
        }

        if (strategy->Entry->handle_tick)
            strategy->Entry->handle_tick(strategy->Instance, strategy, tick);
    }
    // TODO: generate minute bar, and try notify to each strategy
}

static void _zs_strategy_handle_bar(zs_event_engine_t* ee, void* userdata,
    uint32_t evtype, void* evdata)
{
    // 处理行情事件(是否需要优化为多线程)
    zs_strategy_engine_t*   zse;
    zs_cta_strategy_t*      strategy;
    zs_cta_strategy_t*      strategy_array[MAX_STRATEGY_COUNT];
    zs_data_head_t*         zdh;
    zs_bar_t*               bar;
    zs_sid_t                sid;
    uint32_t                count;

    zse = (zs_strategy_engine_t*)userdata;
    zdh = (zs_data_head_t*)evdata;
    bar = (zs_bar_t*)zd_data_body(zdh);
    sid = bar->Sid;

    count = _zs_strategy_retrieve(zse, sid, strategy_array, MAX_STRATEGY_COUNT);
    for (uint32_t i = 0; i < count; ++i)
    {
        strategy = (zs_cta_strategy_t*)strategy_array[i];
        if (!strategy)
        {
            continue;
        }

        zs_bar_reader_t* bar_reader = NULL;
        memcpy(&bar_reader, bar, sizeof(void*));

        if (strategy->Entry->handle_bar)
            strategy->Entry->handle_bar(strategy->Instance, strategy, bar_reader);
    }
}

/* zs_strategy engine */
zs_strategy_engine_t* zs_strategy_engine_create(zs_algorithm_t* algo)
{
    zs_strategy_engine_t* zse;

    zse = (zs_strategy_engine_t*)ztl_pcalloc(algo->Pool, sizeof(zs_strategy_engine_t));

    zse->Algorithm      = algo;
    zse->AssetFinder    = algo->AssetFinder;
    zse->Pool           = algo->Pool;
    zse->StrategyBaseID = 1;

    zse->StrategyEntries = ztl_pcalloc(zse->Pool, sizeof(ztl_array_t));
    ztl_array_init(zse->StrategyEntries, NULL, 64, sizeof(zs_strategy_entry_t*));

    zse->AllStrategy = ztl_pcalloc(zse->Pool, sizeof(ztl_array_t));
    ztl_array_init(zse->AllStrategy, NULL, MAX_STRATEGY_COUNT, sizeof(zs_cta_strategy_t*));

    zse->StrategyMap        = ztl_map_create(MAX_STRATEGY_COUNT);

    zse->Tick2StrategyList  = dictCreate(&uintHashDictType, zse);
    zse->BarGenDict         = dictCreate(&uintHashDictType, zse);
    zse->OrderStrategyDict  = zs_orderdict_create(zse->Pool);
    zse->AccountStrategyDict= dictCreate(&strHashDictType, zse);

    zs_strategy_register_event(zse);

    return zse;
}

void zs_strategy_engine_release(zs_strategy_engine_t* zse)
{
    if (!zse) {
        return;
    }

    if (zse->StrategyEntries) {
        ztl_array_release(zse->StrategyEntries);
        zse->StrategyEntries = NULL;
    }

    if (zse->AllStrategy) {
        ztl_array_release(zse->AllStrategy);
        zse->AllStrategy = NULL;
    }

    if (zse->StrategyMap) {
        ztl_map_release(zse->StrategyMap);
        zse->StrategyMap = NULL;
    }

    if (zse->Tick2StrategyList) {
        dictRelease(zse->Tick2StrategyList);
        zse->Tick2StrategyList = NULL;
    }

    if (zse->BarGenDict) {
        dictRelease(zse->BarGenDict);
        zse->BarGenDict = NULL;
    }

    if (zse->OrderStrategyDict) {
        zs_orderdict_release(zse->OrderStrategyDict);
        zse->OrderStrategyDict = NULL;
    }

    if (zse->AccountStrategyDict) {
        dictRelease(zse->AccountStrategyDict);
        zse->AccountStrategyDict = NULL;
    }

}

void zs_strategy_engine_stop(zs_strategy_engine_t* zse)
{
    //
}

static void zs_strategy_register_event(zs_strategy_engine_t* zse)
{
    zs_event_engine_t* ee = zse->Algorithm->EventEngine;

    zs_ee_register(ee, zse, ZS_DT_Order, _zs_strategy_handle_order);
    zs_ee_register(ee, zse, ZS_DT_Trade, _zs_strategy_handle_trade);
    zs_ee_register(ee, zse, ZS_DT_MD_Tick, _zs_strategy_handle_tick);
    zs_ee_register(ee, zse, ZS_DT_MD_Bar, _zs_strategy_handle_bar);
}


int zs_strategy_engine_load(zs_strategy_engine_t* zse, ztl_array_t* libpaths)
{
    // load all strategy entries from dso

    const char* libpath;
    for (uint32_t i = 0; i < ztl_array_size(libpaths); ++i)
    {
        libpath = ztl_array_at(libpaths, i);
        zs_strategy_load(zse, libpath);
    }

#if 0
    // load demo strategy
    zs_strategy_entry_t* entry = NULL;
    zs_demo_strategy_entry(&entry);
    entry->Instance = entry->create(0);

    ztl_map_add(zse->StrategyMap, 16, entry);
    void** dst = ztl_array_push(&zse->AllStrategy);
    *dst = entry;
#endif

    return 0;
}

int zs_strategy_load(zs_strategy_engine_t* zse, const char* libpath)
{
    zs_strategy_entry_t* entry;
    ztl_dso_handle_t* dso;

    dso = ztl_dso_load(libpath);
    if (!dso)
    {
        // ERRORID: log error
        return -1;
    }

    // retrieve strategy entry func from dso
    zs_strategy_entry_pt entry_func;
    entry_func = ztl_dso_symbol(dso, zs_strategy_entry_name);
    if (!entry_func)
    {
        // ERRORID: not defined entry func
        ztl_dso_unload(dso);
        return -2;
    }

    entry_func(&entry);
    if (!entry)
    {
        // ERRORID: not got entry object
        ztl_dso_unload(dso);
        return -3;
    }

#if 0
    entry = (zs_strategy_entry_t*)ztl_pcalloc(zse->Pool, sizeof(zs_strategy_entry_t));
    entry->HLib         = dso;
    entry->create       = ztl_dso_symbol(dso, "create");
    entry->release      = ztl_dso_symbol(dso, "release");
    entry->get_version  = ztl_dso_symbol(dso, "get_version");
    entry->get_info     = ztl_dso_symbol(dso, "get_info");
    entry->on_init      = ztl_dso_symbol(dso, "on_init");
    entry->on_start     = ztl_dso_symbol(dso, "on_start");
    entry->on_stop      = ztl_dso_symbol(dso, "on_stop");
    entry->on_update    = ztl_dso_symbol(dso, "on_update");
    entry->on_timer     = ztl_dso_symbol(dso, "on_timer");
    entry->before_trading_start = ztl_dso_symbol(dso, "before_trading_start");

    entry->handle_order = ztl_dso_symbol(dso, "handle_order");
    entry->handle_trade = ztl_dso_symbol(dso, "handle_trade");
    entry->handle_bar   = ztl_dso_symbol(dso, "handle_bar");
    entry->handle_tick  = ztl_dso_symbol(dso, "handle_tick");
    entry->handle_tickl2= ztl_dso_symbol(dso, "handle_tickl2");

    if (entry->get_version) {
        entry->Version = entry->get_version(0);
    }
    if (entry->get_info) {
        entry->get_info(entry->StrategyName, entry->Author, 0);
    }
    else {
        // error ?
    }
#endif//0

    entry->HLib = dso;

    void** pdst = ztl_array_push(zse->StrategyEntries);
    *pdst = entry;

    return 0;
}

int zs_strategy_unload(zs_strategy_engine_t* zse, zs_strategy_entry_t* entry)
{
    ztl_dso_handle_t* dso;

    dso = (ztl_dso_handle_t*)entry->HLib;

    if (dso) {
        ztl_dso_unload(dso);
        // entry->HLib = NULL;
    }

    return 0;
}

int zs_strategy_create(zs_strategy_engine_t* zse, zs_cta_strategy_t** pstrategy, 
    const char* strategy_name, const char* setting)
{
    // 创建一个策略对象
    zs_cta_strategy_t*      strategy;
    zs_strategy_entry_t*    entry;

    // 是否已存在，FIXME：同时加载多个相同名的策略时如何处理？
    strategy = ztl_array_find(zse->AllStrategy, (char*)strategy_name, _cta_strategy_name_comp);
    if (strategy)
    {
        *pstrategy = strategy;
        return 0;
    }

    entry = ztl_array_find(zse->StrategyEntries, (char*)strategy_name, _strategy_entry_name_comp);
    if (!entry)
    {
        // ERRORID: not find the strategy entry
        return -1;
    }

    uint32_t next_strategy_id = zse->StrategyBaseID++;
    strategy = zs_cta_strategy_create(zse, setting, next_strategy_id);

    zs_cta_strategy_set_entry(strategy, entry);

    // 调用策略初始化函数
    if (strategy->Entry->create) {
        strategy->Instance = strategy->Entry->create(strategy, setting);
    }

    *pstrategy = strategy;

    return 0;
}

int zs_strategy_add(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    ztl_vector_t* vec;

    // add to map and array

    if (!ztl_array_find(zse->AllStrategy, strategy, _cta_strategy_object_comp)) {
        ztl_array_push_back(zse->AllStrategy, strategy);
    }

    if (!ztl_map_find(zse->StrategyMap, (uint64_t)strategy->StrategyID)) {
        ztl_map_add(zse->StrategyMap, strategy->StrategyID, strategy);
    }

    ZStrKey key = { (int)strlen(strategy->pAccountID), (char*)strategy->pAccountID };
    vec = (ztl_vector_t*)dictFind(zse->AccountStrategyDict, &key);
    if (!vec) {
        vec = ztl_vector_create(MAX_STRATEGY_COUNT, sizeof(zs_cta_strategy_t*));

        ZStrKey* pkey = (ZStrKey*)ztl_pcalloc(zse->Pool, ztl_align(sizeof(ZStrKey) + key.len, sizeof(void*)));
        dictAdd(zse->AccountStrategyDict, pkey, strategy);
    }
    vec->push_ptr(vec, strategy);

    return 0;
}

int zs_strategy_del(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    zs_strategy_stop(zse, strategy);

    // TODO: remove this strategy from memory

    return 0;
}

int zs_strategy_init(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    // 初始化策略
    if (strategy->Entry->on_init)
        strategy->Entry->on_init(strategy, strategy);

    return 0;
}

int zs_strategy_start(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    if (strategy->Entry->on_start) {
        strategy->Entry->on_start(strategy->Instance, strategy);
    }

    return 0;
}

int zs_strategy_stop(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    if (strategy->Entry->on_stop) {
        strategy->Entry->on_stop(strategy->Instance, strategy);
    }

    return 0;
}

int zs_strategy_update(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, const char* new_setting)
{
    if (strategy->Entry->on_update) {
        strategy->Entry->on_update(strategy->Instance, strategy, (void*)new_setting, (int)strlen(new_setting));
    }

    return 0;
}

int zs_strategy_find_by_sid(zs_strategy_engine_t* zse, zs_sid_t sid, zs_cta_strategy_t* strategy_array[], int size)
{
    int index = 0;
    dictEntry* entry;
    // zs_cta_strategy_t* strategy;

    entry = dictFind(zse->Tick2StrategyList, (void*)sid);
    if (entry)
    {
        // ztl_dlist_t* dlist = (ztl_dlist_t*)entry->v.val;
    }

    return index;
}

int zs_strategy_find_by_name(zs_strategy_engine_t* zse, const char* strategy_name, zs_cta_strategy_t* strategy_array[], int size)
{
    return 0;
}

zs_cta_strategy_t* zs_strategy_find(zs_strategy_engine_t* zse, uint32_t strategy_id)
{
    zs_cta_strategy_t* strategy;
    strategy = ztl_map_find(zse->StrategyMap, strategy_id);
    return strategy;
}

int zs_strategy_engine_save_order(zs_strategy_engine_t* zse, 
    zs_cta_strategy_t* strategy, zs_order_req_t* order_req)
{
    return 0;
}


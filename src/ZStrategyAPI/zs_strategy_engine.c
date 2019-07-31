
#include <ZToolLib/ztl_dyso.h>
#include <ZToolLib/ztl_memcpy.h>

#include "zs_algorithm.h"
#include "zs_assets.h"
#include "zs_constants_helper.h"
#include "zs_data_portal.h"
#include "zs_hashdict.h"
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

/* zs_strategy event handlers */
static void _zs_strategy_handle_order(zs_event_engine_t* ee, zs_strategy_engine_t* zse,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 处理订单回报事件
    zs_cta_strategy_t*  strategy;
    zs_order_t*         order;

    order = (zs_order_t*)zd_data_body(evdata);

    zs_log_debug(zse->Log, "ctas: handle_order symbol:%s,qty:%d,px:%.2lf,dir:%d,offset:%d",
        order->Symbol, order->OrderQty, order->OrderPrice, order->Direction, order->OffsetFlag);

    // find the strategy related the order
    strategy = zs_orderdict_find(zse->OrderStrategyDict, order->FrontID, order->SessionID, order->OrderID);
    if (strategy && strategy->Entry->handle_order) {
        strategy->Entry->handle_order(strategy->Instance, strategy, order);
    }
}

static void _zs_strategy_handle_trade(zs_event_engine_t* ee, zs_strategy_engine_t* zse,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 处理成交事件
    zs_cta_strategy_t*      strategy;
    zs_trade_t*             trade;
    dictEntry*              entry;
    zs_sid_t                sid;
    char                    zs_tradeid[32];

    trade = (zs_trade_t*)zd_data_body(evdata);
    sid = trade->Sid;

    int len = zs_make_id(zs_tradeid, trade->ExchangeID, trade->TradeID);
    entry = zs_strdict_find(zse->TradeDict, zs_tradeid, len);
    if (entry) {
        // 重复的成交回报
        return;
    }

    zs_log_debug(zse->Log, "ctas: handle_trade symbol:%s,qty:%d,px:%.2lf,dir:%d,offset:%d",
        trade->Symbol, trade->Volume, trade->Price, trade->Direction, trade->OffsetFlag);

    // no need make a copy of trade object here
    ZStrKey* key;
    key = zs_str_keydup2(zs_tradeid, len, ztl_palloc, zse->Pool);
    dictAdd(zse->TradeDict, key, trade);

    // since trade's frontid & session was filled when blotter process trade
    strategy = zs_orderdict_find(zse->OrderStrategyDict, trade->FrontID, trade->SessionID, trade->OrderID);
    if (strategy && strategy->Entry->handle_trade) {
        strategy->Entry->handle_trade(strategy->Instance, strategy, trade);
    }
}

static void _zs_strategy_handle_tick(zs_event_engine_t* ee, zs_strategy_engine_t* zse,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 处理行情事件(是否需要优化为多线程)
    ztl_array_t*        strategy_array;
    zs_cta_strategy_t*  strategy;
    zs_tick_t*          tick;
    zs_sid_t            sid;
    dictEntry*          entry;

    tick = (zs_tick_t*)zd_data_body(evdata);
    sid = tick->Sid;

    // fprintf(stderr, "cta_handle_tick %s,%d\n", tick->Symbol, tick->UpdateTime);
    zs_log_trace(zse->Log, "ctas: handle_tick symbol:%s,lastpx:%.2lf,vol:%ld,time:%d",
        tick->Symbol, tick->LastPrice, tick->Volume, tick->UpdateTime);

    entry = dictFind(zse->Tick2StrategyList, (void*)sid);
    if (!entry) {
        return;
    }

    strategy_array = (ztl_array_t*)entry->v.val;
    for (int index = 0; index < (int)ztl_array_size(strategy_array); ++index)
    {
        strategy = (zs_cta_strategy_t*)ztl_array_at2(strategy_array, index);
        if (!strategy) {
            continue;
        }

        if (strategy->Entry->handle_tick) {
            strategy->Entry->handle_tick(strategy->Instance, strategy, tick);
        }
    }

    // TODO: generate minute bar, and try notify to each strategy
}

static void _zs_strategy_handle_bar(zs_event_engine_t* ee, zs_strategy_engine_t* zse,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 处理行情事件(是否需要优化为多线程)
    ztl_array_t*        strategy_array;
    zs_cta_strategy_t*  strategy;
    zs_bar_t*           bar;
    zs_sid_t            sid;
    dictEntry*          entry;

    bar = (zs_bar_t*)zd_data_body(evdata);
    sid = bar->Sid;

    entry = dictFind(zse->Tick2StrategyList, (void*)sid);
    if (!entry) {
        return;
    }

    zs_log_trace(zse->Log, "ctas: handle_bar symbol:%s,o:%.2lf,h:%.2lf,l:%.2lf,c:%.2lf,time:%ld",
        bar->Symbol, bar->OpenPrice, bar->HighPrice, bar->LowPrice, bar->ClosePrice, bar->BarTime);

    strategy_array = (ztl_array_t*)entry->v.val;
    for (int index = 0; index < (int)ztl_array_size(strategy_array); ++index)
    {
        strategy = (zs_cta_strategy_t*)ztl_array_at2(strategy_array, index);
        if (!strategy) {
            continue;
        }

        zs_bar_reader_t bar_reader = { 0 };
        ztl_memcpy(&bar_reader.Bar, bar, sizeof(zs_bar_t));

        if (strategy->Entry->handle_bar) {
            strategy->Entry->handle_bar(strategy->Instance, strategy, &bar_reader);
        }
    }
}

static void _zs_strategy_handle_timer(zs_event_engine_t* ee, zs_strategy_engine_t* zse,
    uint32_t evtype, zs_data_head_t* evdata)
{
    uint32_t i;
    zs_cta_strategy_t* strategy;

    for (i = 0; i < ztl_array_size(zse->AllStrategy); ++i)
    {
        strategy = (zs_cta_strategy_t*)ztl_array_at2(zse->AllStrategy, i);
        if (!strategy) {
            continue;
        }

        if (strategy->Entry->on_timer) {
            strategy->Entry->on_timer(strategy->Instance, strategy, 0);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
/* zs_strategy engine */
zs_strategy_engine_t* zs_strategy_engine_create(zs_algorithm_t* algo)
{
    zs_strategy_engine_t* zse;

    zse = (zs_strategy_engine_t*)ztl_pcalloc(algo->Pool, sizeof(zs_strategy_engine_t));

    zse->Algorithm      = algo;
    zse->AssetFinder    = algo->AssetFinder;
    zse->Pool           = algo->Pool;
    zse->Log            = algo->Log;
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

    zse->TradeDict          = dictCreate(&strHashDictType, zse);

    // FIXME:
    zs_strategy_engine_load(zse, NULL);

    zs_strategy_register_event(zse);

    return zse;
}

void zs_strategy_engine_release(zs_strategy_engine_t* zse)
{
    if (!zse) {
        return;
    }

    zs_log_info(zse->Log, "ctas: engine_release");

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

    if (zse->TradeDict) {
        dictRelease(zse->TradeDict);
        zse->TradeDict = NULL;
    }

}

void zs_strategy_engine_stop(zs_strategy_engine_t* zse)
{
    uint32_t i;
    zs_cta_strategy_t* strategy;

    zs_log_info(zse->Log, "ctas: engine_stop");

    for (i = 0; i < ztl_array_size(zse->AllStrategy); ++i)
    {
        strategy = (zs_cta_strategy_t*)ztl_array_at2(zse->AllStrategy, i);
        if (!strategy) {
            continue;
        }

        zs_strategy_stop(zse, strategy);
    }
}

static void zs_strategy_register_event(zs_strategy_engine_t* zse)
{
    zs_event_engine_t* ee;
    if (!zse->Algorithm || !zse->Algorithm->EventEngine) {
        return;
    }
    ee = zse->Algorithm->EventEngine;

    zs_ee_register(ee, zse, ZS_DT_Order, _zs_strategy_handle_order);
    zs_ee_register(ee, zse, ZS_DT_Trade, _zs_strategy_handle_trade);
    zs_ee_register(ee, zse, ZS_DT_MD_Tick, _zs_strategy_handle_tick);
    zs_ee_register(ee, zse, ZS_DT_MD_Bar, _zs_strategy_handle_bar);
    zs_ee_register(ee, zse, ZS_DT_Timer, _zs_strategy_handle_timer);
}


int zs_strategy_engine_load(zs_strategy_engine_t* zse, ztl_array_t* libpaths)
{
    // load all strategy entries from dso

    const char* libpath;

    if (libpaths)
    {
        for (uint32_t i = 0; i < ztl_array_size(libpaths); ++i)
        {
            libpath = ztl_array_at2(libpaths, i);
            zs_strategy_load(zse, libpath);
        }
    }

    return 0;
}

int zs_strategy_load(zs_strategy_engine_t* zse, const char* libpath)
{
    zs_strategy_entry_t* strategy_entry;
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
        return ZS_ERR_NoEntryFunc;
    }

    entry_func(&strategy_entry);
    if (!strategy_entry)
    {
        // ERRORID: not got entry object
        ztl_dso_unload(dso);
        return ZS_ERR_NoEntryObject;
    }

#if 0
    strategy_entry = (zs_strategy_entry_t*)ztl_pcalloc(zse->Pool, sizeof(zs_strategy_entry_t));
    strategy_entry->HLib         = dso;
    strategy_entry->create       = ztl_dso_symbol(dso, "create");
    strategy_entry->release      = ztl_dso_symbol(dso, "release");
    strategy_entry->get_version  = ztl_dso_symbol(dso, "get_version");
    strategy_entry->get_info     = ztl_dso_symbol(dso, "get_info");
    strategy_entry->on_init      = ztl_dso_symbol(dso, "on_init");
    strategy_entry->on_start     = ztl_dso_symbol(dso, "on_start");
    strategy_entry->on_stop      = ztl_dso_symbol(dso, "on_stop");
    strategy_entry->on_update    = ztl_dso_symbol(dso, "on_update");
    strategy_entry->on_timer     = ztl_dso_symbol(dso, "on_timer");
    strategy_entry->before_trading_start = ztl_dso_symbol(dso, "before_trading_start");

    strategy_entry->handle_order = ztl_dso_symbol(dso, "handle_order");
    strategy_entry->handle_trade = ztl_dso_symbol(dso, "handle_trade");
    strategy_entry->handle_bar   = ztl_dso_symbol(dso, "handle_bar");
    strategy_entry->handle_tick  = ztl_dso_symbol(dso, "handle_tick");
    strategy_entry->handle_tickl2= ztl_dso_symbol(dso, "handle_tickl2");

    if (strategy_entry->get_version) {
        strategy_entry->Version = strategy_entry->get_version(0);
    }
    else {
        // error ?
    }
#endif//0

    strategy_entry->HLib = dso;

    zs_strategy_entry_add(zse, strategy_entry);

    return ZS_OK;
}

int zs_strategy_unload(zs_strategy_engine_t* zse, zs_strategy_entry_t* strategy_entry)
{
    ztl_dso_handle_t* dso;

    dso = (ztl_dso_handle_t*)strategy_entry->HLib;

    if (dso)
    {
        ztl_dso_unload(dso);
        // strategy_entry->HLib = NULL;
    }

    return ZS_OK;
}

int zs_strategy_entry_add(zs_strategy_engine_t* zse, zs_strategy_entry_t* strategy_entry)
{
    if (!strategy_entry->StrategyName) {
        return -1;
    }

    for (uint32_t x = 0; x < ztl_array_size(zse->StrategyEntries); ++x)
    {
        zs_strategy_entry_t* temp = ztl_array_at2(zse->StrategyEntries, x);
        if (strcmp(temp->StrategyName, strategy_entry->StrategyName) == 0) {
            return ZS_EXISTED;
        }
    }

    zs_log_info(zse->Log, "cta: strategy_entry_add name:%s, ptr:%p",
        strategy_entry->StrategyName, strategy_entry);

    ztl_array_push_back(zse->StrategyEntries, &strategy_entry);
    return ZS_OK;
}

ztl_array_t* zs_strategy_get_entries(zs_strategy_engine_t* zse)
{
    return zse->StrategyEntries;
}

zs_strategy_entry_t* zs_strategy_get_entry(zs_strategy_engine_t* zse, const char* strategy_name)
{
    zs_strategy_entry_t* strategy_entry;
    zs_strategy_entry_t* temp;

    strategy_entry = NULL;
    for (uint32_t x = 0; x < ztl_array_size(zse->StrategyEntries); ++x)
    {
        temp = ztl_array_at2(zse->StrategyEntries, x);
        if (temp && strcmp(temp->StrategyName, strategy_name) == 0) {
            return strategy_entry;
        }
    }
    return NULL;
}


int zs_strategy_create(zs_strategy_engine_t* zse, zs_cta_strategy_t** pstrategy, 
    const char* strategy_name, const char* setting)
{
    // 创建一个策略对象
    zs_cta_strategy_t*      strategy;
    zs_strategy_entry_t*    strategy_entry;
    zs_blotter_t*           blotter;
    uint32_t                next_strategy_id;

#if 0
    // 是否已存在，FIXME：同时加载多个相同名的策略时如何处理？
    strategy = ztl_array_find(zse->AllStrategy, (char*)strategy_name, _cta_strategy_name_comp);
    if (strategy)
    {
        *pstrategy = strategy;
        return 0;
    }
#endif

    strategy_entry = NULL;
    for (uint32_t x = 0; x < ztl_array_size(zse->StrategyEntries); ++x)
    {
        zs_strategy_entry_t* temp = ztl_array_at2(zse->StrategyEntries, x);
        if (strcmp(temp->StrategyName, strategy_name) == 0) {
            strategy_entry = temp;
            break;
        }
    }

    if (!strategy_entry)
    {
        // ERRORID: not find the strategy entry, maybe load failed before
        zs_log_error(zse->Log, "ctas: strategy_create %s but not find entry", strategy_name);
        return ZS_ERR_NoEntryObject;
    }

    next_strategy_id = zse->StrategyBaseID++;
    strategy = zs_cta_strategy_create(zse, setting, next_strategy_id);
    if (!strategy)
    {
        // ERRORID: create cta strategy object failed
        zs_log_error(zse->Log, "ctas: strategy_create %s create object failed", strategy_name);
        return ZS_ERROR;
    }

    zs_cta_strategy_set_entry(strategy, strategy_entry);

    blotter = zs_get_blotter(zse->Algorithm, strategy->pAccountID);
    if (!blotter)
    {
        // ERRORID: no account engine for loading this strategy
        zs_log_error(zse->Log, "ctas: strategy_create %s get blotter failed by account:%s",
            strategy_name, strategy->pAccountID);
        zs_cta_strategy_release(strategy);
        return ZS_ERR_NoAccount;
    }
    zs_cta_strategy_set_blotter(strategy, blotter);

    // 调用策略初始化函数
    if (strategy->Entry->create) {
        strategy->Instance = strategy->Entry->create(strategy, setting);
    }

    zs_log_info(zse->Log, "ctas: strategy_create object:%p for name:%s, account:%s",
        strategy, strategy_name, strategy->pAccountID);

    *pstrategy = strategy;

    return ZS_OK;
}

int zs_strategy_add(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    ztl_array_t* vec;

    // add to map and array
#if 0// FIXME
    if (!ztl_array_find(zse->AllStrategy, strategy, _cta_strategy_object_comp)) {
        ztl_array_push_back(zse->AllStrategy, strategy);
    }
#else
    ztl_array_push_back(zse->AllStrategy, &strategy);
#endif

    if (!ztl_map_find(zse->StrategyMap, (uint64_t)strategy->StrategyID)) {
        ztl_map_add(zse->StrategyMap, strategy->StrategyID, strategy);
    }

    int len = (int)strlen(strategy->pAccountID);
    vec = (ztl_array_t*)zs_strdict_find(zse->AccountStrategyDict, (char*)strategy->pAccountID, len);
    if (!vec) {
        vec = ztl_pcalloc(zse->Pool, sizeof(ztl_array_t));
        ztl_array_init(vec, NULL, MAX_STRATEGY_COUNT, sizeof(zs_cta_strategy_t*));

        ZStrKey* pkey;
        pkey = zs_str_keydup2(strategy->pAccountID, len, ztl_palloc, zse->Pool);
        dictAdd(zse->AccountStrategyDict, pkey, strategy);
    }
    ztl_array_push_back(vec, &strategy);

    zs_log_info(zse->Log, "ctas: strategy_add name:%s, ptr:%p", 
        strategy->pStrategyName, strategy);

    return ZS_OK;
}

int zs_strategy_del(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    zs_log_info(zse->Log, "ctas: strategy_del name:%s, ptr:%p",
        strategy->pStrategyName, strategy);

    zs_strategy_stop(zse, strategy);

    // TODO: remove this strategy from memory

    return ZS_OK;
}

int zs_strategy_init(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    zs_log_info(zse->Log, "ctas: strategy_init name:%s, ptr:%p",
        strategy->pStrategyName, strategy);

    // 初始化策略
    if (strategy->Entry->on_init)
        strategy->Entry->on_init(strategy, strategy);

    return ZS_OK;
}

int zs_strategy_start(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    zs_log_info(zse->Log, "ctas: strategy_start name:%s, ptr:%p",
        strategy->pStrategyName, strategy);

    // 启动策略
    if (strategy->Entry->on_start) {
        strategy->Entry->on_start(strategy->Instance, strategy);
    }

    return ZS_OK;
}

int zs_strategy_stop(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    zs_log_info(zse->Log, "ctas: strategy_stop name:%s, ptr:%p",
        strategy->pStrategyName, strategy);

    // 停止策略
    if (strategy->Entry->on_stop) {
        strategy->Entry->on_stop(strategy->Instance, strategy);
    }

    return ZS_OK;
}

int zs_strategy_pause(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, int trading_flag)
{
    zs_log_info(zse->Log, "ctas: strategy_stop name:%s, ptr:%p, trading_flag:%d",
        strategy->pStrategyName, strategy, trading_flag);

    // 设置策略标志
    zs_cta_strategy_set_trading_flag(strategy, trading_flag);
    return ZS_OK;
}

int zs_strategy_update(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, const char* new_setting)
{
    zs_log_info(zse->Log, "ctas: strategy_update name:%s, ptr:%p",
        strategy->pStrategyName, strategy);

    // 更新策略
    if (strategy->Entry->on_update) {
        strategy->Entry->on_update(strategy->Instance, strategy, (void*)new_setting, (int)strlen(new_setting));
    }

    return ZS_OK;
}

int zs_strategy_init_all(zs_strategy_engine_t* zse, const char* accountid)
{
    zs_cta_strategy_t*  strategy;
    ztl_array_t*        strategy_array;

    zs_log_info(zse->Log, "ctas: init_all accountid:%s", accountid);

    if (!accountid || !accountid[0])
        strategy_array = zse->AllStrategy;
    else
        strategy_array = zs_strategy_find_by_account(zse, accountid);

    for (uint32_t i = 0; i < ztl_array_size(strategy_array); ++i)
    {
        strategy = (zs_cta_strategy_t*)ztl_array_at2(strategy_array, i);
        if (!strategy) {
            continue;
        }

        zs_strategy_init(zse, strategy);
    }

    return ZS_OK;
}

int zs_strategy_start_all(zs_strategy_engine_t* zse, const char* accountid)
{
    zs_cta_strategy_t*  strategy;
    ztl_array_t*        strategy_array;

    zs_log_info(zse->Log, "ctas: start_all accountid:%s", accountid);

    if (!accountid || !accountid[0])
        strategy_array = zse->AllStrategy;
    else
        strategy_array = zs_strategy_find_by_account(zse, accountid);

    for (uint32_t i = 0; i < ztl_array_size(strategy_array); ++i)
    {
        strategy = (zs_cta_strategy_t*)ztl_array_at2(strategy_array, i);
        if (!strategy) {
            continue;
        }

        zs_strategy_start(zse, strategy);
    }

    return ZS_OK;
}

int zs_strategy_stop_all(zs_strategy_engine_t* zse, const char* accountid)
{
    zs_cta_strategy_t*  strategy;
    ztl_array_t*        strategy_array;

    zs_log_info(zse->Log, "ctas: stop_all accountid:%s", accountid);

    if (!accountid || !accountid[0])
        strategy_array = zse->AllStrategy;
    else
        strategy_array = zs_strategy_find_by_account(zse, accountid);

    for (uint32_t i = 0; i < ztl_array_size(strategy_array); ++i)
    {
        strategy = (zs_cta_strategy_t*)ztl_array_at2(strategy_array, i);
        if (!strategy) {
            continue;
        }

        zs_strategy_stop(zse, strategy);
    }

    return ZS_OK;
}

int zs_strategy_del_all(zs_strategy_engine_t* zse, const char* accountid)
{
    // ERRORID: not support currently
    return ZS_ERR_NotImpl;
}

int zs_strategy_put_event(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy)
{
    const char* strategy_info;
    strategy_info = strategy->get_info(strategy);
    if (!strategy_info) {
        // ERRORID: 
        return ZS_ERROR;
    }

    // TODO: 

    return ZS_OK;
}


ztl_array_t* zs_strategy_find_by_sid(zs_strategy_engine_t* zse, zs_sid_t sid)
{
    dictEntry* entry;

    entry = dictFind(zse->Tick2StrategyList, (void*)sid);
    if (entry) {
        return (ztl_array_t*)entry->v.val;
    }

    return NULL;
}

int zs_strategy_find_by_name(zs_strategy_engine_t* zse, const char* strategy_name, 
    zs_cta_strategy_t* strategy_array[], int size)
{
    int index, count;
    zs_cta_strategy_t* strategy;

    index = 0;
    count = 0;
    for (; index < (int)ztl_array_size(zse->AllStrategy) && index < size; ++index)
    {
        strategy = (zs_cta_strategy_t*)ztl_array_at2(zse->AllStrategy, index);
        if (strategy)
            strategy_array[count++] = strategy;
    }

    return count;
}

ztl_array_t* zs_strategy_find_by_account(zs_strategy_engine_t* zse, const char* accountid)
{
    dictEntry* entry;

    entry = zs_strdict_find(zse->AccountStrategyDict, accountid, (int)strlen(accountid));
    if (entry) {
        return (ztl_array_t*)entry->v.val;
    }

    return NULL;
}

zs_cta_strategy_t* zs_strategy_find_byid(zs_strategy_engine_t* zse, uint32_t strategy_id)
{
    zs_cta_strategy_t* strategy;
    strategy = ztl_map_find(zse->StrategyMap, strategy_id);
    return strategy;
}

zs_cta_strategy_t* zs_strategy_find(zs_strategy_engine_t* zse, int frontid, int sessionid, const char* orderid)
{
    zs_cta_strategy_t* strategy;
    strategy = zs_orderdict_find(zse->OrderStrategyDict, frontid, sessionid, orderid);
    return strategy;
}

int zs_strategy_engine_save_order(zs_strategy_engine_t* zse, 
    zs_cta_strategy_t* strategy, zs_order_req_t* order_req)
{
    int rv;
    rv = zs_orderdict_add(zse->OrderStrategyDict, order_req->FrontID, 
            order_req->SessionID, order_req->OrderID, strategy);

    zs_log_debug(zse->Log, "ctas: save_order account:%s, symbol:%s, orderid:%s for strategy:%s",
        order_req->AccountID, order_req->Symbol, order_req->OrderID, strategy->pStrategyName);

    return rv;
}

int zs_strategy_subscribe(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy,
    ZSExchangeID exchangeid, const char* symbol)
{
    ztl_array_t*    array;
    zs_sid_t        sid;
    zs_subscribe_t  sub_req = { 0 };

    zs_log_debug(zse->Log, "ctas: subscribe exchangeid:%s, symbol:%s by strategy:%s",
        exchangeid, symbol, strategy->pStrategyName);

    strcpy(sub_req.Exchange, zs_convert_exchange_id(exchangeid));
    strcpy(sub_req.Symbol, symbol);

    sid = strategy->lookup_sid(strategy, exchangeid, symbol, (int)strlen(symbol));
    if (sid == ZS_SID_INVALID) {
        zs_log_error(zse->Log, "ctas: subscribe fail for exchangeid:%s, symbol:%s by strategy:%s",
            exchangeid, symbol, strategy->pStrategyName);
        return ZS_ERR_NoAsset;
    }

    sub_req.Sid = sid;
    strategy->Blotter->subscribe(strategy->Blotter, &sub_req);

    array = zs_strategy_find_by_sid(zse, sid);
    if (!array) {
        array = (ztl_array_t*)ztl_pcalloc(zse->Pool, sizeof(ztl_array_t));
        ztl_array_init(array, NULL, MAX_STRATEGY_COUNT, sizeof(zs_cta_strategy_t*));
        dictAdd(zse->Tick2StrategyList, (void*)sid, array);
    }

    ztl_array_push_back(array, &strategy);

    return ZS_OK;
}

int zs_strategy_subscribe_bysid(zs_strategy_engine_t* zse, zs_cta_strategy_t* strategy, zs_sid_t sid)
{
    ztl_array_t*    array;
    zs_contract_t*  contract;
    zs_subscribe_t  sub_req = { 0 };

    zs_log_debug(zse->Log, "ctas: subscribe_bysid sid:%d by strategy:%s",
        sid, strategy->pStrategyName);

    contract = strategy->get_contract(strategy, sid);
    if (contract) {
        strcpy(sub_req.Exchange, zs_convert_exchange_id(contract->ExchangeID));
        strcpy(sub_req.Symbol, contract->Symbol);
    }
    sub_req.Sid = sid;

    strategy->Blotter->subscribe(strategy->Blotter, &sub_req);

    array = zs_strategy_find_by_sid(zse, sid);
    if (!array) {
        array = (ztl_array_t*)ztl_pcalloc(zse->Pool, sizeof(ztl_array_t));
        ztl_array_init(array, NULL, MAX_STRATEGY_COUNT, sizeof(zs_cta_strategy_t*));
        dictAdd(zse->Tick2StrategyList, (void*)sid, array);
    }

    ztl_array_push_back(array, &strategy);

    return ZS_OK;
}


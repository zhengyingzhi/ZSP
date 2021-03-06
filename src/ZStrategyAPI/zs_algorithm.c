﻿#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_memcpy.h>

#include "zs_algorithm.h"

#include "zs_assets.h"
#include "zs_blotter.h"
#include "zs_broker_entry.h"
#include "zs_broker_backtest.h"

#include "zs_configs.h"
#include "zs_core.h"
#include "zs_data_portal.h"
#include "zs_event_engine.h"
#include "zs_risk_control.h"
#include "zs_simulator.h"
#include "zs_strategy_engine.h"

#ifdef _MSC_VER
#include <Windows.h>
#define zs_strdup   _strdup
#else
#define zs_strdup   strdup
#endif//_MSC_VER


/* algo event handlers */
static void _zs_algo_handle_order(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_trade(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_qry_order(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_qry_trade(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_position(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_position_detail(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_account(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_contract(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_margin_rate(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_comm_rate(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_timer(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_other(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_tick(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);

static void _zs_algo_handle_bar(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata);



static int _zs_load_strategies(zs_algorithm_t* algo)
{
    zs_strategy_engine_t* zse;
    zse = algo->StrategyEngine;

    if (!zse)
    {
        zse = zs_strategy_engine_create(algo);
        algo->StrategyEngine = zse;
    }

    // 根据配置信息，加载策略

    return ZS_OK;
}

static int _zs_load_brokers(zs_algorithm_t* algo)
{
    // 根据配置文件，加载交易API和行情API

    zs_trade_api_t* tdapi;
    zs_md_api_t*    mdapi;

    if (!algo->Broker) {
        algo->Broker = zs_broker_create(algo);
    }

    if (algo->Params->RunMode == ZS_RM_Backtest)
    {
        // 回测时，则加载backtestapi即可

        tdapi = zs_sim_backtest_trade_api();
        mdapi = zs_sim_backtest_md_api();

        tdapi->UserData = algo;
        mdapi->UserData = algo;

        // tdapi->ApiInstance = tdapi->create("bt_td", 0);
        // mdapi->ApiInstance = mdapi->create("bt_md", 0);

        zs_broker_add_tradeapi(algo->Broker, tdapi);
        zs_broker_add_mdapi(algo->Broker, mdapi);
    }
    else
    {
        tdapi = (zs_trade_api_t*)ztl_pcalloc(algo->Pool, sizeof(zs_trade_api_t));
        mdapi = (zs_md_api_t*)ztl_pcalloc(algo->Pool, sizeof(zs_md_api_t));

#if 1
        // example code, default import ctp interface
        const char* tdlibpath;
        const char* mdlibpath;
        // tdlibpath = "D:\\MyProjects\\ZStrategyPlatform\\build\\msvc\\x64\\Debug\\zs_ctp_trade.dll";

        tdlibpath = "zs_ctp_trade.dll";
        mdlibpath = "zs_ctp_md.dll";

        tdapi->UserData = algo;
        mdapi->UserData = algo;

        zs_broker_trade_load(tdapi, tdlibpath);
        zs_broker_md_load(mdapi, mdlibpath);
#else
        // example demo
        zs_bt_trade_api_entry(tdapi);
        zs_bt_md_api_entry(mdapi);
#endif

        if (tdapi->APIName) {
            zs_log_info(algo->Log, "algo: load_broker add_tradeapi %s\n", tdapi->APIName);
            zs_broker_add_tradeapi(algo->Broker, tdapi);
        }
        if (mdapi->APIName) {
            zs_log_info(algo->Log, "algo: load_broker add_mdapi %s\n", mdapi->APIName);
            zs_broker_add_mdapi(algo->Broker, mdapi);
        }
    }
    return ZS_OK;
}

static int _zs_blotter_connect(zs_algorithm_t* algo)
{
    zs_blotter_t* blotter;
    for (uint32_t i = 0; i < ztl_array_size(algo->BlotterMgr.BlotterArray); ++i)
    {
        blotter = (zs_blotter_t*)ztl_array_at2(algo->BlotterMgr.BlotterArray, i);
        if (!blotter) {
            continue;
        }

        zs_blotter_trade_connect(blotter);
        zs_blotter_md_connect(blotter);
    }
    return ZS_OK;
}

//////////////////////////////////////////////////////////////////////////

/* 交易核心
 * 创建事件引擎，创建核心管理，
 * 加载策略并保存
 * 注册交易核心事件，风控事件，注册策略事件
 * 加载各接口zs_broker_entry_t(可根据名字获取接口实例)
 */
void zs_algorithm_register(zs_algorithm_t* algo);

zs_algorithm_t* zs_algorithm_create(zs_algo_param_t* algo_param)
{
    ztl_pool_t* pool;
    zs_algorithm_t* algo;

    pool = ztl_create_pool(ZTL_DEFAULT_POOL_SIZE);
    algo = (zs_algorithm_t*)ztl_pcalloc(pool, sizeof(zs_algorithm_t));
    algo->Pool = pool;
    algo->Params = algo_param;

    // create log
    bool is_async = algo_param->LogAsync ? true : false;
    if (algo_param->LogName) {
        algo->Log = zs_log_create(algo_param->LogName, ZTL_WritFile, is_async);
    }
    else {
        algo->Log = zs_log_create("", ZTL_PrintScrn, is_async);
    }

    // create other objects...
    algo->EventEngine   = zs_ee_create(algo->Pool, algo->Params->RunMode);
    algo->DataPortal    = NULL;
    algo->Simulator     = NULL;
    zs_blotter_manager_init(&algo->BlotterMgr, algo);
    algo->AssetFinder   = zs_asset_create(algo, algo->Pool, 0);
    algo->StrategyEngine= zs_strategy_engine_create(algo);
    algo->Broker        = zs_broker_create(algo);
    algo->RiskControl   = NULL;

    // 加载API
    _zs_load_brokers(algo);

    return algo;
}

void zs_algorithm_release(zs_algorithm_t* algo)
{
    if (!algo) {
        return;
    }

    zs_log_info(algo->Log, "zs_algorithm_release");

    zs_algorithm_stop(algo);

    // release objects...
    if (algo->EventEngine) {
        zs_ee_release(algo->EventEngine);
        algo->EventEngine = NULL;
    }

    if (algo->AssetFinder) {
        zs_asset_release(algo->AssetFinder);
        algo->AssetFinder = NULL;
    }

    if (algo->DataPortal) {
        zs_data_portal_release(algo->DataPortal);
        algo->DataPortal = NULL;
    }

    zs_blotter_manager_release(&algo->BlotterMgr);

    if (algo->Simulator) {
        zs_simulator_release(algo->Simulator);
        algo->Simulator = NULL;
    }

    if (algo->Category) {
        zs_category_release(algo->Category);
        algo->Category = NULL;
    }

    if (algo->StrategyEngine) {
        zs_strategy_engine_release(algo->StrategyEngine);
        algo->StrategyEngine = NULL;
    }

    if (algo->Broker) {
        zs_broker_release(algo->Broker);
        algo->Broker = NULL;
    }

    if (algo->RiskControl) {
        zs_risk_control_release(algo->RiskControl);
        algo->RiskControl = NULL;
    }

    ztl_destroy_pool(algo->Pool);
}

int zs_algorithm_init(zs_algorithm_t* algo)
{
    zs_log_info(algo->Log, "algo: init");

    algo->Category = (zs_category_t*)ztl_pcalloc(algo->Pool, sizeof(zs_category_t));
    zs_category_init(algo->Category);
    zs_category_load(algo->Category, "category.json");

    zs_strategy_init_all(algo->StrategyEngine, NULL);
    return ZS_OK;
}

void zs_algorithm_register(zs_algorithm_t* algo)
{
    zs_event_engine_t* ee = algo->EventEngine;

    zs_ee_register(ee, algo, ZS_DT_Order, _zs_algo_handle_order);
    zs_ee_register(ee, algo, ZS_DT_Trade, _zs_algo_handle_trade);
    zs_ee_register(ee, algo, ZS_DT_QryPosition, _zs_algo_handle_position);
    zs_ee_register(ee, algo, ZS_DT_QryPositionDetail, _zs_algo_handle_position_detail);
    zs_ee_register(ee, algo, ZS_DT_QryAccount, _zs_algo_handle_account);
    zs_ee_register(ee, algo, ZS_DT_QryContract, _zs_algo_handle_contract);
    zs_ee_register(ee, algo, ZS_DT_MD_Tick, _zs_algo_handle_tick);
    zs_ee_register(ee, algo, ZS_DT_MD_KLine, _zs_algo_handle_bar);
    zs_ee_register(ee, algo, ZS_DT_Timer, _zs_algo_handle_timer);
    zs_ee_register(ee, algo, ZS_DT_Other, _zs_algo_handle_other);
}

int zs_algorithm_run(zs_algorithm_t* algo, zs_data_portal_t* data_portal)
{
    zs_log_info(algo->Log, "algo: running");

    algo->Running = ZS_RS_Running;
    // algo->DataPortal = data_portal;

#if 0
    // fake contract info
    zs_contract_t* contract;
    contract = (zs_contract_t*)ztl_pcalloc(algo->Pool, sizeof(zs_contract_t));
    strcpy(contract->Symbol, "rb1910");
    contract->ExchangeID = ZS_EI_SHFE;
    contract->ProductClass = ZS_PC_Future;
    contract->Multiplier = 10;
    contract->PriceTick = 1.0;

    zs_sid_t sid;
    zs_asset_add(algo->AssetFinder, &sid, contract->ExchangeID, contract->Symbol,
        (int)strlen(contract->Symbol), contract);
    contract->Sid = sid;
#endif//0

    /* 回测：
     * 0. 创建事件引擎
     * 1. 创建相关数据管理模块，及接口，!!注册相关事件及回调函数!!
     * 2. 创建simulator，需要：数据，交易日历，起止时间
     * 3. 加载策略（自动订阅，查询数据）
     * 4. 运行
     */

    // 注册交易核心所关心的事件
    zs_algorithm_register(algo);

    // 加载策略并初始化（策略加载后，也需要注册策略关心的事件：订单事件，成交事件，行情事件）
    _zs_load_strategies(algo);
    zs_strategy_start_all(algo->StrategyEngine, NULL);

    // 启动事件引擎
    zs_ee_start(algo->EventEngine);

    if (algo->Params->RunMode == ZS_RM_Backtest)
    {
        // 运行模拟回测（先post一个looponce事件给引擎，引擎开始工作，然后不停的回调simulator的一个回调函数）
        // simu的回调函数循环取每一个交易时间单元，取出当前时间点的行情，生成各个事件 -->> 
        // a.先给slippage b.再给broker_md_api
        // 这样，回测均在一个(事件引擎)线程中完成：时间->数据->事件->Slippage,brokermdapi->(异同步都ok)->交易核心->策略
        zs_simulator_t* simu;
        simu = zs_simulator_create(algo);

        zs_trade_api_t* tdapi = zs_broker_get_tradeapi(algo->Broker, NULL);
        tdapi->ApiInstance = tdapi->create("", 0);
        tdapi->regist(tdapi->ApiInstance, &td_handlers, tdapi, NULL);
        tdapi->connect(tdapi->ApiInstance, simu->Slippage);

        zs_md_api_t* mdapi = zs_broker_get_mdapi(algo->Broker, NULL);
        mdapi->ApiInstance = mdapi->create("", 0);
        mdapi->regist(mdapi->ApiInstance, &md_handlers, mdapi, NULL);
        mdapi->connect(mdapi->ApiInstance, NULL);

        zs_simulator_regist_tradeapi(simu, tdapi, &td_handlers);
        zs_simulator_regist_mdapi(simu, mdapi, &md_handlers);
        algo->Simulator = simu;

        if (data_portal)
            zs_simulator_set_mdseries(simu, data_portal->RawDatas, data_portal->IsTick);
        zs_simulator_run(simu);
    }
    else
    {
        // 针对每个账户，启动各账户的API连接，则可开始交易
        // 之后自动触发connect, login, marketdata 等事件

        // TODO: 调用各个account的api连接接口

#if 0
        zs_trading_conf_t* trading_conf;
        for (uint32_t i = 0; i < ztl_array_size(&algo->Params->TradingConf); ++i)
        {
            trading_conf = (zs_trading_conf_t*)ztl_array_at2(&algo->Params->TradingConf, 0);

            // 每个账户使用一个交易API和行情API，可直接从配置中获取该账户的账户信息，使用接口信息等
            // 可支持：策略中登录了多个账户（每个账户属于不同的经纪商，如期现同时交易），虽账号不同，但属于同一人
            // 因此，需要支持智能获取接口（如下期货合约，不传入accountID，智能获取ctp，下股票，智能获取tdx接口）
            zs_trade_api_t* tdapi = zs_broker_get_tradeapi(algo->Broker, trading_conf->TradeConf.APIName);
            tdapi->regist(tdapi->ApiInstance, &td_handlers, tdapi, &trading_conf->TradeConf);
            tdapi->connect(tdapi->ApiInstance, NULL);
        }
#else
        _zs_blotter_connect(algo);
#endif//0
}

    // 最后，生成交易报告（回测和实盘均可）

    return ZS_OK;
}

int zs_algorithm_stop(zs_algorithm_t* algo)
{
    if (!algo->Running) {
        return ZS_ERR_NotStarted;
    }

    zs_log_info(algo->Log, "algo: running");

    algo->Running = ZS_RS_Stopping;

    // TODO: stop the compenents

    if (algo->StrategyEngine) {
        zs_strategy_engine_stop(algo->StrategyEngine);
    }

    algo->Running = ZS_RS_Stopped;

    return ZS_OK;
}

int zs_algorithm_result(zs_algorithm_t* algo, ztl_array_t* results)
{
    return ZS_ERR_NotImpl;
}

zs_blotter_t* zs_get_blotter(zs_algorithm_t* algo, const char* accountid)
{
    return zs_blotter_manager_get(&algo->BlotterMgr, accountid);
}

int zs_algorithm_add_strategy_entry(zs_algorithm_t* algo, zs_strategy_entry_t* strategy_entry)
{
    if (!algo->StrategyEngine) {
        algo->StrategyEngine = zs_strategy_engine_create(algo);
    }

    return zs_strategy_entry_add(algo->StrategyEngine, strategy_entry);
}

int zs_algorithm_add_strategy(zs_algorithm_t* algo, const char* strategy_setting)
{
    zs_json_t* zjson;
    zjson = zs_json_parse(strategy_setting, (int)strlen(strategy_setting));
    if (!zjson) {
        // ERRORID: invalid json buffer
        return ZS_ERR_JsonData;
    }

    char acccount_id[32] = "";
    char strategy_name[256] = "";
    zs_json_get_string(zjson, "AccountID", acccount_id, sizeof(acccount_id));
    zs_json_get_string(zjson, "StrategyName", strategy_name, sizeof(strategy_name));

    // the blotter
    zs_blotter_t* blotter;
    blotter = zs_get_blotter(algo, acccount_id);
    if (!blotter)
    {
        // create 
        blotter = zs_blotter_create(algo, acccount_id);
        if (!blotter)
        {
            zs_log_error(algo->Log, "algo: add_strategy create blotter failed for %s\n", acccount_id);
            zs_json_release(zjson);
            return ZS_ERROR;
        }
        zs_log_info(algo->Log, "algo: dd_strategy create blotter for %s\n", acccount_id);

        zs_blotter_manager_add(&algo->BlotterMgr, blotter);
    }

    // the strategy
    int rv;
    zs_cta_strategy_t* strategy = NULL;
    rv = zs_strategy_create(algo->StrategyEngine, &strategy, strategy_name, strategy_setting);
    if (rv == 0 && strategy)
    {
        zs_strategy_add(algo->StrategyEngine, strategy);
    }
    else
    {
        zs_log_error(algo->Log, "algo: add_strategy create strategy:%s failed\n", strategy_name);
        zs_json_release(zjson);
        return ZS_ERROR;
    }

    zs_json_release(zjson);
    return ZS_OK;
}

int zs_algorithm_add_account(zs_algorithm_t* algo, const char* account_setting)
{
    zs_conf_account_t account_conf = { 0 };

    // parse the setting buffer
    zs_json_t* zjson;
    zjson = zs_json_parse(account_setting, (int)strlen(account_setting));
    if (!zjson) {
        // ERRORID: invalid json buffer
        return -1;
    }

    char authcode[512] = "";
    zs_json_get_string(zjson, "AccountID", account_conf.AccountID, sizeof(account_conf.AccountID));
    zs_json_get_string(zjson, "AccountName", account_conf.AccountName, sizeof(account_conf.AccountName));
    zs_json_get_string(zjson, "Password", account_conf.Password, sizeof(account_conf.Password));
    zs_json_get_string(zjson, "BrokerID", account_conf.BrokerID, sizeof(account_conf.BrokerID));
    zs_json_get_string(zjson, "TradeAPIName", account_conf.TradeAPIName, sizeof(account_conf.TradeAPIName));
    zs_json_get_string(zjson, "MDAPIName", account_conf.MDAPIName, sizeof(account_conf.MDAPIName));
    zs_json_get_string(zjson, "AppID", account_conf.AppID, sizeof(account_conf.AppID));
    zs_json_get_string(zjson, "AuthCode", account_conf.AuthCode, sizeof(authcode));
    if (authcode[0]) {
        account_conf.AuthCode = zs_strdup(authcode);
    }

    zs_json_release(zjson);

    return zs_algorithm_add_account2(algo, &account_conf);
}

int zs_algorithm_add_account2(zs_algorithm_t* algo, const zs_conf_account_t* account_conf)
{
    zs_conf_account_t* old_account_conf;
    old_account_conf = zs_configs_find_account(algo->Params, account_conf->AccountID);
    if (old_account_conf) {
        // already exists
        *old_account_conf = *account_conf;
        return 1;
    }

    ztl_array_push_back(&algo->Params->AccountConf, (void*)&account_conf);
    return ZS_OK;
}

int zs_algorithm_add_broker_info(zs_algorithm_t* algo, const char* broker_setting)
{
    zs_conf_broker_t broker_info = { 0 };

    zs_json_t* zjson;
    zjson = zs_json_parse(broker_setting, (int)strlen(broker_setting));
    if (!zjson) {
        // ERRORID: invalid json buffer
        return ZS_ERR_JsonData;
    }

    char authcode[512] = "";
    zs_json_get_string(zjson, "APIName", broker_info.APIName, sizeof(broker_info.APIName));
    zs_json_get_string(zjson, "BrokerID", broker_info.BrokerID, sizeof(broker_info.BrokerID));
    zs_json_get_string(zjson, "BrokerName", broker_info.BrokerName, sizeof(broker_info.BrokerName));
    zs_json_get_string(zjson, "TradeAddr", broker_info.TradeAddr, sizeof(broker_info.TradeAddr));
    zs_json_get_string(zjson, "MDAddr", broker_info.MDAddr, sizeof(broker_info.MDAddr));

    zs_json_release(zjson);

    return zs_algorithm_add_broker_info2(algo, &broker_info);
}

int zs_algorithm_add_broker_info2(zs_algorithm_t* algo, const zs_conf_broker_t* broker_conf)
{
    zs_conf_broker_t* old_broker_conf;
    old_broker_conf = zs_configs_find_broker(algo->Params, broker_conf->BrokerID);
    if (old_broker_conf) {
        // already exist, overwrite old
        *old_broker_conf = *broker_conf;
        return ZS_EXISTED;
    }
    ztl_array_push_back(&algo->Params->BrokerConf, (void*)&broker_conf);
    return ZS_OK;
}


const char* zs_version(int* pver)
{
    if (pver)
        *pver = ZS_Version_int;
    return ZS_Version;
}

//////////////////////////////////////////////////////////////////////////

static void _zs_algo_handle_order(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 订单回报事件
    zs_blotter_t*   blotter;
    zs_order_t*     order;

    order = (zs_order_t*)zd_data_body(evdata);

    blotter = zs_get_blotter(algo, order->AccountID);
    if (blotter) {
        blotter->handle_order_rtn(blotter, order);
    }
}

static void _zs_algo_handle_trade(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 成交回报事件
    zs_blotter_t*   blotter;
    zs_trade_t*     trade;

    trade = (zs_trade_t*)zd_data_body(evdata);

    blotter = zs_get_blotter(algo, trade->AccountID);
    if (blotter) {
        blotter->handle_trade_rtn(blotter, trade);
    }
}

static void _zs_algo_handle_qry_order(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 订单查询事件
    zs_blotter_t*   blotter;
    zs_order_t*     order;

    order = (zs_order_t*)zd_data_body(evdata);

    blotter = zs_get_blotter(algo, order->AccountID);
    if (blotter) {
        zs_blotter_handle_qry_order(blotter, order);
    }
}

static void _zs_algo_handle_qry_trade(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 成交回报事件
    zs_blotter_t*   blotter;
    zs_trade_t*     trade;

    trade = (zs_trade_t*)zd_data_body(evdata);

    blotter = zs_get_blotter(algo, trade->AccountID);
    if (blotter) {
        zs_blotter_handle_qry_trade(blotter, trade);
    }
}

static void _zs_algo_handle_position(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 持仓事件
    zs_blotter_t*   blotter;
    zs_position_t*  pos;

    pos = (zs_position_t*)zd_data_body(evdata);

    blotter = zs_get_blotter(algo, pos->AccountID);
    if (blotter) {
        zs_blotter_handle_position(blotter, pos);
    }
}

static void _zs_algo_handle_position_detail(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 持仓事件
    zs_blotter_t*   blotter;
    zs_position_detail_t*  pos_detail;

    pos_detail = (zs_position_detail_t*)zd_data_body(evdata);

    blotter = zs_get_blotter(algo, pos_detail->AccountID);
    if (blotter) {
        zs_blotter_handle_position_detail(blotter, pos_detail);
    }
}

static void _zs_algo_handle_account(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 资金账户事件
    zs_blotter_t*   blotter;
    zs_fund_account_t* fund_account;

    fund_account = (zs_fund_account_t*)zd_data_body(evdata);

    blotter = zs_get_blotter(algo, fund_account->AccountID);
    if (blotter) {
        zs_blotter_handle_account(blotter, fund_account);
    }
}

static void _zs_algo_handle_contract(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 合约事件
    zs_contract_t* contract, *dup_contract;
    zs_sid_t sid;

    contract = (zs_contract_t*)zd_data_body(evdata);

    dup_contract = (zs_contract_t*)ztl_palloc(algo->Pool, sizeof(zs_contract_t));
    ztl_memcpy(dup_contract, contract, sizeof(zs_contract_t));

    zs_asset_add(algo->AssetFinder, &sid, contract->ExchangeID,
        contract->Symbol, (int)strlen(contract->Symbol), dup_contract);
}

static void _zs_algo_handle_margin_rate(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 保证金事件
    zs_contract_t*      contract;
    zs_margin_rate_t*   margin_rate;

    margin_rate = (zs_margin_rate_t*)zd_data_body(evdata);

    contract = (zs_contract_t*)zs_asset_find(algo->AssetFinder, margin_rate->ExchangeID,
        margin_rate->Symbol, (int)strlen(margin_rate->Symbol));
    if (!contract) {
        // ERRORID: not find contract info for this margin rate
        zs_log_warn(algo->Log, "algo handle_margin_rate no contract info for exchangeid:%d, symbol:%s",
            margin_rate->ExchangeID, margin_rate->Symbol);
        return;
    }

    contract->LongMarginRateByMoney = margin_rate->LongMarginRateByMoney;
    contract->LongMarginRateByVolume = margin_rate->LongMarginRateByVolume;
    contract->ShortMarginRateByMoney = margin_rate->ShortMarginRateByMoney;
    contract->ShortMarginRateByVolume = margin_rate->ShortMarginRateByVolume;
}

static void _zs_algo_handle_comm_rate(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 费率事件
    zs_contract_t* contract;
    zs_commission_rate_t* comm_rate;

    comm_rate = (zs_commission_rate_t*)zd_data_body(evdata);

    contract = (zs_contract_t*)zs_asset_find(algo->AssetFinder, comm_rate->ExchangeID,
        comm_rate->Symbol, (int)strlen(comm_rate->Symbol));
    if (!contract) {
        // ERRORID: not find contract info for this commission rate
        zs_log_warn(algo->Log, "algo handle_comm_rate no contract info for exchangeid:%d, symbol:%s",
            comm_rate->ExchangeID, comm_rate->Symbol);
        return;
    }

    contract->OpenRatioByMoney = comm_rate->OpenRatioByMoney;
    contract->OpenRatioByVolume = comm_rate->OpenRatioByVolume;
    contract->CloseRatioByMoney = comm_rate->CloseRatioByMoney;
    contract->CloseRatioByVolume = comm_rate->CloseRatioByVolume;
    contract->CloseTodayRatioByMoney = comm_rate->CloseTodayRatioByMoney;
    contract->CloseTodayRatioByVolume = comm_rate->CloseTodayRatioByVolume;
}

static void _zs_algo_handle_timer(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // Timer事件
    zs_blotter_t* blotter;

    for (uint32_t i = 0; i < ztl_array_size(algo->BlotterMgr.BlotterArray); ++i)
    {
        blotter = (zs_blotter_t*)ztl_array_at2(algo->BlotterMgr.BlotterArray, i);
        if (blotter)
            zs_blotter_handle_timer(blotter, 0);
    }

}

static void _zs_algo_handle_other(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // 其它事件
}


static void _zs_algo_handle_tick(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // tick事件
    zs_tick_t*      tick;
    zs_blotter_t*   blotter;

    tick = (zs_tick_t*)zd_data_body(evdata);

    static int count = 0;
    count += 1;
    if (count & 127)
        fprintf(stderr, "algo_handle_tick %s,%d\n", tick->Symbol, tick->UpdateTime);

    for (uint32_t i = 0; i < ztl_array_size(algo->BlotterMgr.BlotterArray); ++i)
    {
        blotter = (zs_blotter_t*)ztl_array_at2(algo->BlotterMgr.BlotterArray, i);
        if (blotter)
            blotter->handle_tick(blotter, tick);
    }

}

static void _zs_algo_handle_bar(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    zs_blotter_t*       blotter;
    zs_bar_t*           bar;
    zs_bar_reader_t*    bar_reader;

    bar = (zs_bar_t*)zd_data_body(evdata);
    ztl_memcpy(&bar_reader, bar, sizeof(void*));

    zs_log_trace(algo->Log, "algo: handler_bar symbol:%s, close:%.2lf, time:%ld\n", bar->Symbol, bar->ClosePrice, bar->BarTime);

    for (uint32_t i = 0; i < ztl_array_size(algo->BlotterMgr.BlotterArray); ++i)
    {
        blotter = (zs_blotter_t*)ztl_array_at2(algo->BlotterMgr.BlotterArray, i);
        if (blotter)
            blotter->handle_bar(blotter, bar_reader);
    }

}


int zs_algorithm_session_start(zs_algorithm_t* algo, zs_bar_reader_t* current_data)
{
    return ZS_OK;
}

int zs_algorithm_session_before_trading(zs_algorithm_t* algo, zs_bar_reader_t* current_data)
{
    // TODO: update trading day also
    return ZS_OK;
}

int zs_algorithm_session_every_bar(zs_algorithm_t* algo, zs_bar_reader_t* current_data)
{
    // TODO: notify to finance and strategy model
    return ZS_OK;
}

int zs_algorithm_session_end(zs_algorithm_t* algo, zs_bar_reader_t* current_data)
{
    // TODO: output perf message
    return ZS_OK;
}

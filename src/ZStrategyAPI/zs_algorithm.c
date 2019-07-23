#include <ZToolLib/ztl_array.h>
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

#include <Windows.h>

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

    // 加载策略

    // !example code!
    const char* setting = "{\"symbol\": \"rb1910\", \"strategy_name\": \"strategy_demo\", \"account_id\": \"00100002\"}";
    zs_cta_strategy_t* strategy = NULL;
    zs_strategy_create(zse, &strategy, "strategy_demo", setting);
    if (setting) {
        zs_strategy_add(zse, strategy);
    }

    // init and start strategy
    zs_strategy_init(zse, strategy);
    zs_strategy_start(zse, strategy);

    return 0;
}

static int _zs_load_brokers(zs_algorithm_t* algo)
{
    // 根据配置文件，加载交易API和行情API

    zs_trade_api_t* tdapi;
    zs_md_api_t*    mdapi;

    if (!algo->Broker)
    {
        algo->Broker = zs_broker_create(algo);
    }

    if (algo->Params->RunMode == ZS_RM_Backtest)
    {
        // 回测时，则加载backtestapi即可

        tdapi = zs_sim_backtest_trade_api();
        mdapi = zs_sim_backtest_md_api();

        tdapi->UserData = algo;
        mdapi->UserData = algo;

        tdapi->ApiInstance = tdapi->create("bt_td", 0);
        mdapi->ApiInstance = mdapi->create("bt_md", 0);

        zs_broker_add_tradeapi(algo->Broker, tdapi);
        zs_broker_add_mdapi(algo->Broker, mdapi);
    }
    else
    {
        tdapi = (zs_trade_api_t*)ztl_pcalloc(algo->Pool, sizeof(zs_trade_api_t));
        mdapi = (zs_md_api_t*)ztl_pcalloc(algo->Pool, sizeof(zs_md_api_t));

#if 1
        // example code
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

        if (tdapi->ApiName) {
            zs_broker_add_tradeapi(algo->Broker, tdapi);
            zs_broker_add_mdapi(algo->Broker, mdapi);
        }
    }
    return 0;
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

    // create other objects...
    algo->EventEngine = zs_ee_create(algo->Pool, algo->Params->RunMode);
    algo->DataPortal = NULL;
    algo->Simulator = NULL;
    zs_blotter_manager_init(&algo->BlotterMgr, algo);
    algo->AssetFinder = zs_asset_create(algo, algo->Pool, 0);
    // algo->StrategyEngine = zs_strategy_engine_create(algo);
    algo->StrategyEngine = NULL;
    algo->Broker = zs_broker_create(algo);
    algo->RiskControl = NULL;

    return algo;
}

void zs_algorithm_release(zs_algorithm_t* algo)
{
    if (!algo)
    {
        return;
    }

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
    return 0;
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
    zs_ee_register(ee, algo, ZS_DT_MD_Bar, _zs_algo_handle_bar);
    zs_ee_register(ee, algo, ZS_DT_Timer, _zs_algo_handle_timer);
    zs_ee_register(ee, algo, ZS_DT_Other, _zs_algo_handle_other);
}

int zs_algorithm_run(zs_algorithm_t* algo, zs_data_portal_t* data_portal)
{
    algo->DataPortal = data_portal;

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

    /* 回测：
     * 0. 创建事件引擎
     * 1. 创建相关数据管理模块，及接口，!!注册相关事件及回调函数!!
     * 2. 创建simulator，需要：数据，交易日历，起止时间
     * 3. 加载策略（自动订阅，查询数据）
     * 4. 运行
     */

     // 加载API
    _zs_load_brokers(algo);

    // 注册交易核心所关心的事件
    zs_algorithm_register(algo);

    // 交易核心管理(当前只有一个)
    zs_blotter_t* blotter;
    // blotter = zs_blotter_create(algo, "00100002");
    blotter = zs_blotter_create(algo, "00028039");
    zs_blotter_manager_add(&algo->BlotterMgr, blotter);

    // 加载策略并初始化（策略加载后，也需要注册策略关心的事件：订单事件，成交事件，行情事件）
    _zs_load_strategies(algo);

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

        // TODO
#if 0
        zs_trading_conf_t* trading_conf;
        trading_conf = (zs_trading_conf_t*)ztl_array_at2(&algo->Params->TradingConf, 0);

        zs_trade_api_t* tdapi = zs_broker_get_tradeapi(algo->Broker, NULL);
        tdapi->regist(tdapi->ApiInstance, &td_handlers, tdapi, &trading_conf->TradeConf);
        tdapi->connect(tdapi->ApiInstance, simu->Slippage);

        zs_md_api_t* mdapi = zs_broker_get_mdapi(algo->Broker, NULL);
        mdapi->regist(mdapi->ApiInstance, &md_handlers, mdapi, &trading_conf->MdConf);
        mdapi->connect(mdapi->ApiInstance, NULL);

        zs_simulator_regist_tradeapi(simu, tdapi, &td_handlers);
        zs_simulator_regist_mdapi(simu, mdapi, &md_handlers);
        algo->Simulator = simu;

#endif
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
            zs_trade_api_t* tdapi = zs_broker_get_tradeapi(algo->Broker, trading_conf->TradeConf.ApiName);
            tdapi->regist(tdapi->ApiInstance, &td_handlers, tdapi, &trading_conf->TradeConf);
            tdapi->connect(tdapi->ApiInstance, NULL);
        }
#else
        zs_trade_api_t* tdapi;
        zs_md_api_t* mdapi;

        // tdapi = zs_broker_get_tradeapi(algo->Broker, "backtest");
        // mdapi = zs_broker_get_mdapi(algo->Broker, "backtest");
        tdapi = zs_broker_get_tradeapi(algo->Broker, "CTP");
        mdapi = zs_broker_get_mdapi(algo->Broker, "CTP");

        tdapi->UserData = algo;
        mdapi->UserData = algo;

        blotter->TradeApi = tdapi;
        blotter->MdApi = mdapi;

        if (tdapi) {
            tdapi->ApiInstance = tdapi->create("", 0);
            tdapi->regist(tdapi->ApiInstance, &td_handlers, tdapi, blotter->account_conf);
            tdapi->connect(blotter->TradeApi->ApiInstance, NULL);
        }
        if (mdapi) {
            mdapi->ApiInstance = mdapi->create("", 0);
            mdapi->regist(mdapi->ApiInstance, &md_handlers, mdapi, blotter->account_conf);
            mdapi->connect(mdapi->ApiInstance, NULL);
        }
#endif//0
}

    // 最后，生成交易报告（回测和实盘均可）

    return 0;
}

int zs_algorithm_stop(zs_algorithm_t* algo)
{
    if (!algo->Running) {
        return 0;
    }

    algo->Running = 0;

    // TODO: stop the compenents

    if (algo->StrategyEngine) {
        zs_strategy_engine_stop(algo->StrategyEngine);
    }

    return 0;
}

int zs_algorithm_result(zs_algorithm_t* algo, ztl_array_t* results)
{
    return 0;
}

zs_blotter_t* zs_get_blotter(zs_algorithm_t* algo, const char* accountid)
{
    return zs_blotter_manager_get(&algo->BlotterMgr, accountid);
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
        blotter->handle_order_returned(blotter, order);
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
        blotter->handle_order_trade(blotter, trade);
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

static void _zs_algo_handle_timer(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    // Timer事件
    zs_blotter_t* blotter;

    for (uint32_t i = 0; i < ztl_array_size(algo->BlotterMgr.BlotterArray); ++i)
    {
        blotter = (zs_blotter_t*)ztl_array_at2(algo->BlotterMgr.BlotterArray, i);
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

    fprintf(stderr, "algo_handle_tick %s,%d\n", tick->Symbol, tick->UpdateTime);

    for (uint32_t i = 0; i < ztl_array_size(algo->BlotterMgr.BlotterArray); ++i)
    {
        blotter = (zs_blotter_t*)ztl_array_at2(algo->BlotterMgr.BlotterArray, i);
        blotter->handle_tick(blotter, tick);
    }

}

static void _zs_algo_handle_bar(zs_event_engine_t* ee, zs_algorithm_t* algo,
    uint32_t evtype, zs_data_head_t* evdata)
{
    zs_blotter_t*   blotter;

    zs_bar_reader_t* bar_reader;
    memcpy(&bar_reader, zd_data_body(evdata), sizeof(void*));

    double closepx = bar_reader->current(bar_reader, 16, "close");
    fprintf(stderr, "algo md handler bar close:%.2lf\n", closepx);

    for (uint32_t i = 0; i < ztl_array_size(algo->BlotterMgr.BlotterArray); ++i)
    {
        blotter = (zs_blotter_t*)ztl_array_at2(algo->BlotterMgr.BlotterArray, i);
        blotter->handle_bar(blotter, bar_reader);
    }

}


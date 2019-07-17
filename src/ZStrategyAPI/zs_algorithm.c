#include <ZToolLib/ztl_array.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_blotter.h"

#include "zs_broker_entry.h"

#include "zs_broker_backtest.h"

#include "zs_configs.h"

#include "zs_core.h"

#include "zs_data_portal.h"

#include "zs_event_engine.h"

#include "zs_simulator.h"

#include "zs_strategy_engine.h"



/* algo event handlers */
static void _zs_algo_handle_order(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    zs_data_head_t* zdh;
    zs_order_t* order;

    zdh = (zs_data_head_t*)evdata;
    order = (zs_order_t*)zd_data_body(zdh);

    zs_blotter_t* blotter;
    blotter = zs_blotter_manager_get(&ee->Algorithm->BlotterMgr, order->AccountID);

    zs_handle_order_returned(blotter, order);
}

static void _zs_algo_handle_trade(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    zs_blotter_t* blotter;
    zs_data_head_t* zdh;
    zs_trade_t* trade;

    zdh = (zs_data_head_t*)evdata;
    trade = (zs_trade_t*)zd_data_body(zdh);

    blotter = zs_blotter_manager_get(&ee->Algorithm->BlotterMgr, trade->AccountID);

    zs_handle_order_trade(blotter, trade);
}

static void _zs_algo_handle_md(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    zs_data_head_t* zdh;
    zdh = (zs_data_head_t*)evdata;

    // 行情事件，分bar和tick
    // 根据最新行情，更新交易核心的最新价格，浮动盈亏等

    // 保存行情到data_portal中，便于程序可随时访问最新价格？

    // how to identify data type???
    if (evtype == ZS_DT_MD_Bar || 1)
    {
        zs_bar_reader_t* bar_reader;
        memcpy(&bar_reader, zd_data_body(zdh), sizeof(void*));

        double closepx = bar_reader->current(bar_reader, 16, "close");
        printf("algo md handler bar close:%.2lf\n", closepx);

        zs_handle_md_bar(ee->Algorithm, bar_reader);
    }
    else if (evtype == ZS_DT_MD_Tick)
    {
        zs_tick_t* tick_data;
        memcpy(&tick_data, zd_data_body(zdh), sizeof(void*));

        double closepx = tick_data->LastPrice;
        printf("algo md handler tick close:%.2lf\n", closepx);

        zs_handle_md_tick(ee->Algorithm, tick_data);
    }
}

static void _zs_algo_handle_position(zs_event_engine_t* ee, void* userData,
    uint32_t evtype, void* evdata)
{
    // 持仓事件
}

static void _zs_algo_handle_account(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    // 账户事件
}

static void _zs_algo_handle_contract(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    // 合约事件
}

static void _zs_algo_handle_timer(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    // Timer事件
}

static void _zs_algo_handle_other(zs_event_engine_t* ee, void* userData, 
    uint32_t evtype, void* evdata)
{
    // 其它事件
}


static int _zs_load_strategies(zs_algorithm_t* algo)
{
    zs_strategy_engine_t* zse;
    zse = algo->StrategyEngine;

    if (!zse)
    {
        zse = zs_strategy_engine_create(algo);
        algo->StrategyEngine = zse;
    }

    //ztl_array_t* stgLibNames;
    //stgLibNames = &algo->Params->StrategyNames;
    // ztl_array_t stgLibArray;

    // 加载所有策略，内部会自动注册策略模拟所需的事件（订单，成交，行情事件）
    //zs_strategy_engine_load(zse, &stgLibArray);

    return 0;
}

static int _zs_make_assets_info(zs_asset_finder_t* AssetFinder, 
    zs_data_portal_t* dataPortal)
{
    // 回测时，把数据放到assetFinder的map中，便于查找
    // 可再封装提供一个函数，处理每一个合约信息
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

        // example code
        const char* tdlibpath;
        const char* mdlibpath;
        tdlibpath = "zs_ctp_trade.so";
        mdlibpath = "zs_ctp_md.so";

        tdapi->UserData = algo;
        mdapi->UserData = algo;

        zs_broker_trade_load(tdapi, tdlibpath);
        zs_broker_md_load(mdapi, mdlibpath);

        zs_broker_add_tradeapi(algo->Broker, tdapi);
        zs_broker_add_mdapi(algo->Broker, mdapi);
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

zs_algorithm_t* zs_algorithm_create(zs_algo_param_t* algo_param)
{
    ztl_pool_t* pool;
    zs_algorithm_t* algo;

    pool = ztl_create_pool(ZTL_DEFAULT_POOL_SIZE);
    algo = (zs_algorithm_t*)ztl_pcalloc(pool, sizeof(zs_algorithm_t));
    algo->Pool = pool;
    algo->Params = algo_param;

    // create other objects...
    algo->EventEngine = zs_ee_create(algo);
    algo->Simulator = NULL;
    zs_blotter_manager_init(&algo->BlotterMgr, algo);
    algo->AssetFinder = zs_asset_create(algo, algo->Pool, 0);

    return algo;
}

void zs_algorithm_release(zs_algorithm_t* algo)
{
    if (algo)
    {
        // release other objects...
        zs_ee_release(algo->EventEngine);
        zs_asset_release(algo->AssetFinder);
        ztl_destroy_pool(algo->Pool);
    }
}

int zs_algorithm_init(zs_algorithm_t* algo)
{
    return 0;
}

static void zs_algorithm_register(zs_algorithm_t* algo)
{
    zs_event_engine_t* ee = algo->EventEngine;
    zs_ee_register(ee, algo, ZS_DT_Order, _zs_algo_handle_order);
    zs_ee_register(ee, algo, ZS_DT_Trade, _zs_algo_handle_trade);
    zs_ee_register(ee, algo, ZS_DT_QryPosition, _zs_algo_handle_position);
    zs_ee_register(ee, algo, ZS_DT_QryAccount, _zs_algo_handle_account);
    zs_ee_register(ee, algo, ZS_DT_QryContract, _zs_algo_handle_contract);
    zs_ee_register(ee, algo, ZS_DT_MD_Tick, _zs_algo_handle_md);
    zs_ee_register(ee, algo, ZS_DT_Timer, _zs_algo_handle_timer);
    zs_ee_register(ee, algo, ZS_DT_Other, _zs_algo_handle_other);
}

int zs_algorithm_run(zs_algorithm_t* algo, zs_data_portal_t* data_portal)
{
    algo->DataPortal = data_portal;

    // fake contract info
    zs_contract_t contract = { 0 };
    strcpy(contract.Symbol, "000001.SZSE");
    contract.ExchangeID = ZS_EI_SZSE;
    contract.ProductClass = ZS_PC_Stock;
    contract.Multiplier = 1;
    contract.PriceTick = 0.01f;
    zs_sid_t sid;
    zs_asset_add_copy(algo->AssetFinder, &sid, contract.ExchangeID, contract.Symbol, (int)strlen(contract.Symbol), &contract, sizeof(contract));
    contract.Sid = sid;

    /* 回测：
     * 0. 创建事件引擎
     * 1. 创建相关数据管理模块，及接口，!!注册相关事件及回调函数!!
     * 2. 创建simulator，需要：数据，交易日历，起止时间
     * 3. 加载策略（自动订阅，查询数据）
     * 4. 运行
     */

    // 注册交易核心所关心的事件
    zs_algorithm_register(algo);

    // 交易核心管理(当前只有一个)
    zs_blotter_t* blotter;
    blotter = zs_blotter_create(algo);

    strcpy(blotter->Account->AccountID, "000100000002");
    zs_blotter_manager_add(&algo->BlotterMgr, blotter);

    // 加载策略并初始化（策略加载后，也需要注册策略关心的事件：订单事件，成交事件，行情事件）
    _zs_load_strategies(algo);

    // 加载API
    _zs_load_brokers(algo);

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
        trading_conf = (zs_trading_conf_t*)ztl_array_at(&algo->Params->TradingConf, 0);

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
            trading_conf = (zs_trading_conf_t*)ztl_array_at(&algo->Params->TradingConf, 0);

            // 每个账户使用一个交易API和行情API，可直接从配置中获取该账户的账户信息，使用接口信息等
            // 可支持：策略中登录了多个账户（每个账户属于不同的经纪商，如期现同时交易），虽账号不同，但属于同一人
            // 因此，需要支持智能获取接口（如下期货合约，不传入accountID，智能获取ctp，下股票，智能获取tdx接口）
            zs_trade_api_t* tdapi = zs_broker_get_tradeapi(algo->Broker, trading_conf->TradeConf.ApiName);
            tdapi->regist(tdapi->ApiInstance, &td_handlers, tdapi, &trading_conf->TradeConf);
            tdapi->connect(tdapi->ApiInstance, NULL);
        }
#endif//0
}

    // 最后，生成交易报告（回测和实盘均可）

    return 0;
}

int zs_algorithm_stop(zs_algorithm_t* algo)
{
    return 0;
}

int zs_algorithm_result(zs_algorithm_t* algo, ztl_array_t* results)
{
    return 0;
}



#include <ZToolLib/ztl_threads.h>
#include <ZToolLib/ztl_utils.h>

#include "zs_algorithm.h"
#include "zs_broker_backtest.h"
#include "zs_simulator.h"
#include "zs_slippage.h"


// deprecated
typedef enum
{
    ZS_SEV_PreRun,
    ZS_SEV_SesstionStart,
    ZS_SEV_Bar,
    ZS_SEV_SessionEnd,
    ZS_SEV_PostRun
}ZSSimuEvent;


// 处理slippage的回调，并回调通知到api_handler(即broker_entry里定义的)
static void _zs_slippage_td_handler(zs_slippage_t* slippage, ZSSlippageDataType datatype,
    void* data, int size);
// 处理模拟器的新行情，并回调通知到api_handler(即broker_entry里定义的)
static void _zs_simu_newbar_handler(zs_simulator_t* simu, zs_bar_t* bar);
static void _zs_simu_newtick_handler(zs_simulator_t* simu, zs_tick_t* tick);

static int zs_simulator_run_ticks(zs_simulator_t* simu);
static int zs_simulator_run_bars(zs_simulator_t* simu);

static void zs_simulator_simu_handler(zs_simulator_t* simu);

//////////////////////////////////////////////////////////////////////////
zs_simulator_t* zs_simulator_create(zs_algorithm_t* algo)
{
    zs_simulator_t* simu;

    simu = (zs_simulator_t*)ztl_pcalloc(algo->Pool, sizeof(zs_simulator_t));

    simu->Algorithm     = algo;
    simu->DataPortal    = algo->DataPortal;
    simu->BacktestConf  = &algo->Params->BacktestConf;

    const char* market_name = (simu->BacktestConf->ProductType == ZS_PC_Future) ? ZS_EXCHANGE_SHFE : ZS_EXCHANGE_SSE;
    simu->TradingCalendar = zs_tc_create_by_market(simu->BacktestConf->StartDate, simu->BacktestConf->EndDate, market_name);

    simu->Slippage = zs_slippage_create(_zs_slippage_td_handler, simu);
    zs_slippage_set_price_field(simu->Slippage, ZS_PFF_Open);

    simu->TdApi = NULL;
    simu->MdApi = NULL;

    simu->Progress = 0;
    simu->Running = ZS_RS_Inited;

    return simu;
}

void zs_simulator_release(zs_simulator_t* simu)
{
    if (simu)
    {
        if (simu->Slippage) {
            zs_slippage_release(simu->Slippage);
            simu->Slippage = NULL;
        }
    }
}

void zs_simulator_regist_tradeapi(zs_simulator_t* simu,
    zs_trade_api_t* tdapi, zs_trade_api_handlers_t* td_handlers)
{
    simu->TdApi = tdapi;
    simu->TdHandlers = td_handlers;
}

void zs_simulator_regist_mdapi(zs_simulator_t* simu,
    zs_md_api_t* mdapi, zs_md_api_handlers_t* md_handlers)
{
    simu->MdApi = mdapi;
    simu->MdHandlers = md_handlers;
}

void zs_simulator_set_mdseries(zs_simulator_t* simu, ztl_array_t* mdseries, int32_t istick)
{
    simu->MdSeries = mdseries;
    simu->IsTickMd = istick;
}

void zs_simulator_stop(zs_simulator_t* simu)
{
    simu->Running = ZS_RS_Stopped;
}

int zs_simulator_run(zs_simulator_t* simu)
{
    simu->Running = ZS_RS_Running;

    // 模拟API事件
    zs_simulator_simu_handler(simu);

#if 0
    /* 根据起止日期，从交易日历的时间信息，及回测时间段，然后生成各个事件
     * 循环获取交易日历中的各个日期（更新currentdt），然后依次触发各种回测事件
     */

    zs_bar_reader_t     current_data;
    int64_t             current_dt;

    zs_bar_reader_init(&current_data, simu->Algorithm->DataPortal);
    current_dt = zs_tc_first_session(simu->TradingCalendar);

    while (1)
    {
        // update current data
        current_data.CurrentDt = current_dt;

        // SESSION_START event, mid-night dt
        zs_slippage_session_start(simu->Slippage, &current_data);
        zs_algorithm_session_start(simu->Algorithm, &current_data);

        // BEFORE_TRADING_EVENT event, like 08:45:00 dt
        current_data.CurrentDt = current_dt + (8 * 3600 + 45 * 60);
        zs_slippage_session_before_trading(simu->Slippage, &current_data);
        zs_algorithm_session_before_trading(simu->Algorithm, &current_data);

        // BAR event, 15:00:00 dt if daily
        current_data.CurrentDt = current_dt + (15 * 3600);
        zs_slippage_session_every_bar(simu->Slippage, &current_data);
        zs_algorithm_session_every_bar(simu->Algorithm, &current_data);

        // SESSION_END event, mid-night dt as well ?
        zs_slippage_session_end(simu->Slippage, &current_data);
        zs_algorithm_session_end(simu->Algorithm, &current_data);

        current_dt += ZS_SECONDS_PER_DAY;
    }
    return ZS_OK;
#endif//0

    if (simu->MdSeries)
    {
        if (simu->IsTickMd) {
            zs_simulator_run_ticks(simu);
        }
        else {
            zs_simulator_run_bars(simu);
        }
    }
    else
    {
        return ZS_ERROR;
    }

    simu->Running = ZS_RS_Stopped;

    return ZS_OK;
}


static void zs_simulator_simu_handler(zs_simulator_t* simu)
{
    zs_conf_backtest_t* bktconf;
    bktconf = simu->BacktestConf;

    // simulate notify some api events
    zs_trade_api_handlers_t* td_handlers;
    td_handlers = simu->TdHandlers;

    zs_error_data_t err = { 0 };

    // 连接成功
    if (td_handlers->on_connect)
    {
        td_handlers->on_connect(simu->TdApi);
    }

    // 登录成功
    if (td_handlers->on_login)
    {
        zs_login_t login = { 0 };
        strcpy(login.BrokerID, ZS_BACKTEST_BROKERID);
        strcpy(login.AccountID, ZS_BACKTEST_ACCOUNTID);
        login.Result = 0;
        td_handlers->on_login(simu->TdApi, &login, &err);
    }

    // 资金信息
    if (td_handlers->on_qry_trading_account)
    {
        zs_fund_account_t fa = { 0 };
        strcpy(fa.BrokerID, ZS_BACKTEST_BROKERID);
        strcpy(fa.AccountID, ZS_BACKTEST_ACCOUNTID);
        fa.PreBalance = bktconf->CapitalBase;
        fa.Balance = bktconf->CapitalBase;
        fa.Available = bktconf->CapitalBase;
        fa.TradingDay = (int32_t)0;
        td_handlers->on_qry_trading_account(simu->TdApi, &fa, &err, 1);
    }
}

static void _zs_slippage_td_handler(zs_slippage_t* slippage, 
    ZSSlippageDataType data_type, void* data, int size)
{
    zs_simulator_t* simu;
    zs_trade_api_handlers_t* td_handlers;

    simu = (zs_simulator_t*)slippage->UserData;
    td_handlers = simu->TdHandlers;

    if (data_type == ZS_SDT_Order)
    {
        zs_order_t* order = (zs_order_t*)data;

        if (td_handlers->on_rtn_order)
            td_handlers->on_rtn_order(simu->TdApi, order);
    }
    else if (data_type == ZS_SDT_Trade)
    {
        zs_trade_t* trade = (zs_trade_t*)data;

        if (td_handlers->on_rtn_trade)
            td_handlers->on_rtn_trade(simu->TdApi, trade);
    }
}


static void _zs_simu_newbar_handler(zs_simulator_t* simu, zs_bar_t* bar)
{
    zs_md_api_handlers_t* md_handlers;
    md_handlers = simu->MdHandlers;

    if (md_handlers->on_rtn_kline)
        md_handlers->on_rtn_kline(simu->MdApi, bar);
}

static void _zs_simu_newtick_handler(zs_simulator_t* simu, zs_tick_t* tick)
{
    zs_md_api_handlers_t* md_handlers;
    md_handlers = simu->MdHandlers;

    if (md_handlers->on_rtn_mktdata)
        md_handlers->on_rtn_mktdata(simu->MdApi, tick);
}

static int zs_simulator_run_bars(zs_simulator_t* simu)
{
    uint32_t    index;
    zs_bar_t*   prev_bar;
    zs_bar_t*   bar;

    prev_bar = NULL;
    bar = (zs_bar_t*)ztl_array_at2(simu->MdSeries, 0);
    zs_slippage_update_tradingday(simu->Slippage, (int32_t)(bar->BarTime / 1000000000));

    // replay each tick
    for (index = 0; index < ztl_array_size(simu->MdSeries); ++index)
    {
        if (simu->Running != ZS_RS_Running) {
            break;
        }

        bar = (zs_bar_t*)ztl_array_at2(simu->MdSeries, index);

#if 0
        if (prev_bar && prev_bar->TradingDay != bar->TradingDay)
        {
            // trading day changed
            zs_slippage_update_tradingday(simu->Slippage, bar->TradingDay);
        }
#endif

        // 获取到新行情，先撮合
        zs_slippage_process_bybar(simu->Slippage, bar);

        // 通知新行情
        _zs_simu_newbar_handler(simu, bar);

        prev_bar = bar;
    }

    fprintf(stderr, "zs_simulator_run_bars finished!\n");
    return ZS_OK;
}

static int zs_simulator_run_ticks(zs_simulator_t* simu)
{
    uint32_t    index;
    zs_tick_t*  prev_tick;
    zs_tick_t*  tick;

    prev_tick = NULL;
    tick = (zs_tick_t*)ztl_array_at2(simu->MdSeries, 0);
    zs_slippage_update_tradingday(simu->Slippage, tick->TradingDay);

    // replay each tick
    for (index = 0; index < ztl_array_size(simu->MdSeries); ++index)
    {
        if (simu->Running != ZS_RS_Running) {
            break;
        }

        tick = (zs_tick_t*)ztl_array_at2(simu->MdSeries, index);

        if (prev_tick && prev_tick->TradingDay != tick->TradingDay)
        {
            // trading day changed
            zs_slippage_update_tradingday(simu->Slippage, tick->TradingDay);
        }

        // 获取到新行情，先撮合
        zs_slippage_process_bytick(simu->Slippage, tick);

        // 通知新行情
        _zs_simu_newtick_handler(simu, tick);

        prev_tick = tick;
    }

    fprintf(stderr, "zs_simulator_run_ticks finished!\n");
    return ZS_OK;
}

//////////////////////////////////////////////////////////////////////////
zs_trade_api_t* zs_sim_backtest_trade_api()
{
    return &bt_tdapi;
}

zs_md_api_t* zs_sim_backtest_md_api()
{
    return &bt_mdapi;
}

#include "zs_algorithm.h"

#include "zs_broker_entry.h"

#include "zs_event_engine.h"


/* global broker trade api handlers
 */
static void zs_td_on_connect(zs_trade_api_t* tdapi);
static void zs_td_on_disconnect(zs_trade_api_t* tdapi, int reason);
static void zs_td_on_login(zs_trade_api_t* tdapi, zs_login_t* login_rsp, zs_error_data_t* errdata);
static void zs_td_on_logout(zs_trade_api_t* tdapi, zs_logout_t* logout_rsp, zs_error_data_t* errdata);
static void zs_td_on_rtn_order(zs_trade_api_t* tdapi, zs_order_t* order);
static void zs_td_on_rtn_trade(zs_trade_api_t* tdapi, zs_trade_t* trade);
static void zs_td_on_rtn_data(void* tdctx, int dtype, void* data, int size);
static void zs_td_on_rsp_data(void* tdctx, int dtype, void* data, int size, zs_error_data_t* errdata, uint32_t flag);


zs_trade_api_handlers_t td_handlers = {
    zs_td_on_connect,
    zs_td_on_disconnect,
    zs_td_on_login,
    zs_td_on_logout,
    zs_td_on_rtn_order,
    zs_td_on_rtn_trade,
    zs_td_on_rtn_data,
    zs_td_on_rsp_data
};


/* global broker md handlers
 */
static void zs_md_on_connect(zs_md_api_t* mdapi);
static void zs_md_on_disconnect(zs_md_api_t* mdapi, int reason);
static void zs_md_on_login(zs_md_api_t* mdapi, zs_login_t* login_rsp, zs_error_data_t* errdata);
static void zs_md_on_logout(zs_md_api_t* mdapi, zs_logout_t* logout_rsp, zs_error_data_t* errdata);
static void zs_md_on_subscribe(zs_md_api_t* mdapi, zs_subscribe_t* sub_rsp, int flag);
static void zs_md_on_unsubscribe(zs_md_api_t* mdapi, zs_subscribe_t* unsub_rsp, int flag);
static void zs_md_on_rtn_mktdata(zs_md_api_t* mdapi, zs_tick_t* tick);
static void zs_md_on_rtn_mktdatal2(zs_md_api_t* mdapi, zs_tickl2_t* tickl2);
static void zs_md_on_for_quote(zs_md_api_t* mdapi, void* forquote);

zs_md_api_handlers_t md_handlers = {
    zs_md_on_connect,
    zs_md_on_disconnect,
    zs_md_on_login,
    zs_md_on_logout,
    zs_md_on_subscribe,
    zs_md_on_unsubscribe,
    zs_md_on_rtn_mktdata,
    zs_md_on_rtn_mktdatal2,
    zs_md_on_for_quote
};



static void _zs_free(zs_data_head_t* zdh)
{
    free(zdh);
}

//////////////////////////////////////////////////////////////////////////
zs_broker_t* zs_broker_create(zs_algorithm_t* algo)
{
    ztl_pool_t* pool;
    zs_broker_t* broker;

    pool = algo->Pool;
    broker = (zs_broker_t*)ztl_pcalloc(pool, sizeof(zs_broker_t));

    broker->Algorithm = algo;

    return broker;
}

void zs_broker_release(zs_broker_t* broker)
{
    if (broker)
    {
        //
    }
}

int zs_broker_add_tradeapi(zs_broker_t* broker, zs_trade_api_t* tradeapi)
{
    for (int i = 0; i < ZS_MAX_API_NUM; ++i)
    {
        if (!broker->TradeApis[i])
        {
            broker->TradeApis[i] = tradeapi;
            return 0;
        }
    }
    return -1;
}

int zs_broker_add_mdapi(zs_broker_t* broker, zs_md_api_t* mdapi)
{
    for (int i = 0; i < ZS_MAX_API_NUM; ++i)
    {
        if (!broker->MdApis[i])
        {
            broker->MdApis[i] = mdapi;
            return 0;
        }
    }
    return -1;
}

zs_trade_api_t* zs_broker_get_tradeapi(zs_broker_t* broker, const char* apiName)
{
    if (!apiName) {
        return broker->TradeApis[0];
    }

    for (int i = 0; i < ZS_MAX_API_NUM; ++i)
    {
        if (!broker->TradeApis[i]) {
            continue;
        }

        if (strcmp(broker->TradeApis[i]->ApiName, apiName) == 0)
            return broker->TradeApis[i];
    }
    return NULL;
}

zs_md_api_t* zs_broker_get_mdapi(zs_broker_t* broker, const char* apiName)
{
    if (!apiName) {
        return broker->MdApis[0];
    }

    for (int i = 0; i < ZS_MAX_API_NUM; ++i)
    {
        if (!broker->MdApis[i]) {
            continue;
        }

        if (strcmp(broker->MdApis[i]->ApiName, apiName) == 0)
            return broker->MdApis[i];
    }
    return NULL;
}



//////////////////////////////////////////////////////////////////////////

static void zs_td_on_connect(zs_trade_api_t* tdapi)
{
    //
}

static void zs_td_on_disconnect(zs_trade_api_t* tdapi, int reason)
{
    //
}

static void zs_td_on_login(zs_trade_api_t* tdapi, zs_login_t* login_rsp, zs_error_data_t* errdata)
{
    //
}

static void zs_td_on_logout(zs_trade_api_t* tdapi, zs_logout_t* logout_rsp, zs_error_data_t* errdata)
{
    //
}

static void zs_td_on_rtn_order(zs_trade_api_t* tdapi, zs_order_t* order)
{
    // 收到报单通知

    int rv;
    zs_data_head_t* zdh;
    zs_event_engine_t* ee;
    zs_order_t* dst_order;

    zdh = (zs_data_head_t*)malloc(zd_data_head_size + sizeof(zs_order_t));
    memset(zdh, 0, zd_data_head_size + sizeof(zs_order_t));

    dst_order = (zs_order_t*)zd_data_body(zdh);
    *(dst_order) = *(order);

    zdh->pSymbol = dst_order->Symbol;
    zdh->SymbolLength = (int)strlen(dst_order->Symbol);
    zdh->Cleanup = _zs_free;

    // how to visit ee
    ee = ((zs_algorithm_t*)tdapi->UserData)->EventEngine;
    rv = zs_ee_post(ee, ZS_EV_Order, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_rtn_trade(zs_trade_api_t* tdapi, zs_trade_t* trade)
{
    // 收到成交通知
    int rv;
    zs_data_head_t* zdh;
    zs_event_engine_t* ee;
    zs_trade_t* dst_trade;

    zdh = (zs_data_head_t*)malloc(zd_data_head_size + sizeof(zs_trade_t));
    memset(zdh, 0, zd_data_head_size + sizeof(zs_trade_t));

    dst_trade = (zs_trade_t*)zd_data_body(zdh);
    *dst_trade = *trade;

    zdh->pSymbol = dst_trade->Symbol;
    zdh->Cleanup = _zs_free;

    // how to visit ee
    ee = ((zs_algorithm_t*)tdapi->UserData)->EventEngine;
    rv = zs_ee_post(ee, ZS_EV_Trade, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_rtn_data(void* tdctx, int dtype, void* data, int size)
{}

static void zs_td_on_rsp_data(void* tdctx, int dtype, void* data, int size, zs_error_data_t* errdata, uint32_t flag)
{}


static void zs_td_on_qry_account(zs_trade_api_t* tdapi, zs_fund_account_t* account)
{
    // 资金账户

    int rv;
    zs_data_head_t* zdh;
    zs_event_engine_t* ee;
    zs_fund_account_t* dst_account;

    zdh = (zs_data_head_t*)malloc(zd_data_head_size + sizeof(zs_fund_account_t));
    memset(zdh, 0, zd_data_head_size + sizeof(zs_fund_account_t));

    dst_account = (zs_fund_account_t*)zd_data_body(zdh);
    *dst_account = *account;

    zdh->pSymbol = NULL;
    zdh->Cleanup = _zs_free;

    // how to visit ee
    ee = ((zs_algorithm_t*)tdapi->UserData)->EventEngine;
    rv = zs_ee_post(ee, ZS_EV_Account, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_qry_order(zs_trade_api_t* tdapi, zs_order_t* order)
{
    //
}

static void zs_td_on_qry_position(zs_trade_api_t* tdapi, zs_position_t* position)
{
    //
}

static void zs_td_on_qry_position_detail(zs_trade_api_t* tdapi, zs_position_detail_t* pos_detail)
{
    //
}

static void zs_td_on_qry_contract(zs_trade_api_t* tdapi, zs_contract_t* contract)
{
    // 合约通知
    int rv;
    zs_data_head_t* zdh;
    zs_event_engine_t* ee;
    zs_contract_t* dst_contract;

    zdh = (zs_data_head_t*)malloc(zd_data_head_size + sizeof(zs_contract_t));
    memset(zdh, 0, zd_data_head_size + sizeof(zs_contract_t));

    dst_contract = (zs_contract_t*)zd_data_body(zdh);
    *dst_contract = *contract;

    zdh->pSymbol = NULL;
    zdh->Cleanup = _zs_free;

    // how to visit ee
    ee = ((zs_algorithm_t*)tdapi->UserData)->EventEngine;
    rv = zs_ee_post(ee, ZS_EV_Contract, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_exchange_state(zs_trade_api_t* tdapi, const char* exchange, 
        const char* symbol, int state)
{
    //
}


//////////////////////////////////////////////////////////////////////////

static void zs_md_on_connect(zs_md_api_t* mdapi)
{
    //
}

static void zs_md_on_disconnect(zs_md_api_t* mdapi, int reason)
{
    //
}

static void zs_md_on_login(zs_md_api_t* mdapi, zs_login_t* login_rsp, zs_error_data_t* errdata)
{
    //
}

static void zs_md_on_logout(zs_md_api_t* mdapi, zs_logout_t* logout_rsp, zs_error_data_t* errdata)
{
    //
}

static void zs_md_on_subscribe(zs_md_api_t* mdapi, zs_subscribe_t* sub_rsp, int flag)
{
    //
}

static void zs_md_on_unsubscribe(zs_md_api_t* mdapi, zs_subscribe_t* sub_rsp, int flag)
{
    //
}

static void zs_md_on_rtn_mktdata(zs_md_api_t* mdapi, zs_tick_t* tick)
{
    //if (mdapi->MdHandlers->on_marketdata)
    //    mdapi->MdHandlers->on_marketdata(mdapi, tickData);

    // level1行情通知
    int rv;
    zs_data_head_t* zdh;
    zs_event_engine_t* ee;
    zs_tick_t* dst_tick;

    zdh = (zs_data_head_t*)malloc(zd_data_head_size + sizeof(zs_tick_t));
    memset(zdh, 0, zd_data_head_size + sizeof(zs_tick_t));

    dst_tick = (zs_tick_t*)zd_data_body(zdh);
    *dst_tick = *tick;

    zdh->pSymbol = NULL;
    zdh->Cleanup = _zs_free;

    // how to visit ee
    ee = ((zs_algorithm_t*)mdapi->UserData)->EventEngine;
    rv = zs_ee_post(ee, ZS_EV_Contract, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_md_on_rtn_mktdatal2(zs_md_api_t* mdapi, zs_tickl2_t* tickl2)
{
    //
}

static void zs_md_on_for_quote(zs_md_api_t* mdapi, void* forquote)
{
    //
}

#if 0
static void zs_md_on_bardata(zs_md_api_t* mdapi, zs_bar_reader_t* barReader, int reserve)
{
    //if (mdapi->MdHandlers->on_bardata)
    //    mdapi->MdHandlers->on_bardata(mdapi, barData, reserve);

    // bar行情通知(主要是回测数据)
    int rv;
    zs_data_head_t* zdh;
    zs_event_engine_t* ee;

    // make data
    zdh = (zs_data_head_t*)malloc(zd_data_head_size + sizeof(barReader));
    memset(zdh, 0, zd_data_head_size + sizeof(barReader));

    zdh->Cleanup = _zs_free;
    zdh->CtxData = mdapi;
    zdh->RefCount = 1;
    zdh->DType = ZS_DT_MD_Bar;
    zdh->Size = sizeof(barReader);

    memcpy(zd_data_body(zdh), &barReader, sizeof(barReader));

    // post to work thread
    ee = ((zs_algorithm_t*)mdapi->UserData)->EventEngine;
    rv = zs_ee_post(ee, ZS_EV_MD, zdh);
    if (rv != 0)
    {
        // log error
    }
}
#endif


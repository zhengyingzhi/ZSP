#include <ZToolLib/ztl_dyso.h>
#include <ZToolLib/ztl_memcpy.h>

#include <string.h>

#include "zs_algorithm.h"
#include "zs_broker_entry.h"
#include "zs_constants_helper.h"
#include "zs_event_engine.h"

#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif//_MSC_VER



/* global broker trade api handlers
 */
static void zs_td_on_connect(zs_trade_api_t* tdctx);
static void zs_td_on_disconnect(zs_trade_api_t* tdctx, int reason);
static void zs_td_on_error(zs_trade_api_t* tdctx, zs_error_data_t* errdata);
static void zs_td_on_authenticate(zs_trade_api_t* tdctx, zs_authenticate_t* auth_rsp, zs_error_data_t* errdata);
static void zs_td_on_login(zs_trade_api_t* tdctx, zs_login_t* login_rsp, zs_error_data_t* errdata);
static void zs_td_on_logout(zs_trade_api_t* tdctx, zs_logout_t* logout_rsp, zs_error_data_t* errdata);
static void zs_td_on_rtn_order(zs_trade_api_t* tdctx, zs_order_t* order);
static void zs_td_on_rtn_trade(zs_trade_api_t* tdctx, zs_trade_t* trade);
static void zs_td_on_rtn_instrument_status(zs_trade_api_t* tdctx, zs_instrument_status_t* instrument_status);
static void zs_td_on_rtn_data(zs_trade_api_t* tdctx, int dtype, void* data, int size);
static void zs_td_on_qry_contract(zs_trade_api_t* tdctx, zs_contract_t* contract, zs_error_data_t* errdata, uint32_t flag);
static void zs_td_on_qry_trading_account(zs_trade_api_t* tdctx, zs_fund_account_t* fund_account, zs_error_data_t* errdata, uint32_t flag);
static void zs_td_on_qry_order(zs_trade_api_t* tdctx, zs_order_t* order, zs_error_data_t* errdata, uint32_t flag);
static void zs_td_on_qry_trade(zs_trade_api_t* tdctx, zs_trade_t* trade, zs_error_data_t* errdata, uint32_t flag);
static void zs_td_on_qry_position(zs_trade_api_t* tdctx, zs_position_t* pos, zs_error_data_t* errdata, uint32_t flag);
static void zs_td_on_qry_position_detail(zs_trade_api_t* tdctx, zs_position_detail_t* pos_detail, zs_error_data_t* errdata, uint32_t flag);
static void zs_td_on_qry_margin_rate(zs_trade_api_t* tdctx, zs_margin_rate_t* margin_rate, zs_error_data_t* errdata, uint32_t flag);
static void zs_td_on_qry_commission_rate(zs_trade_api_t* tdctx, zs_commission_rate_t* comm_rate, zs_error_data_t* errdata, uint32_t flag);
static void zs_td_on_rsp_data(zs_trade_api_t* tdctx, int dtype, void* data, int size, zs_error_data_t* errdata, uint32_t flag);


zs_trade_api_handlers_t td_handlers = {
    zs_td_on_connect,
    zs_td_on_disconnect,
    zs_td_on_error,
    zs_td_on_authenticate,
    zs_td_on_login,
    zs_td_on_logout,
    NULL,               // settlement confirm
    zs_td_on_rtn_order,
    zs_td_on_rtn_trade,
    zs_td_on_rtn_instrument_status,
    zs_td_on_rtn_data,
    zs_td_on_qry_contract,
    zs_td_on_qry_trading_account,
    zs_td_on_qry_order,
    zs_td_on_qry_trade,
    zs_td_on_qry_position,
    zs_td_on_qry_position_detail,
    zs_td_on_qry_margin_rate,
    zs_td_on_qry_commission_rate,
    zs_td_on_rsp_data
};


/* global broker md handlers
 */
static void zs_md_on_connect(zs_md_api_t* mdctx);
static void zs_md_on_disconnect(zs_md_api_t* mdctx, int reason);
static void zs_md_on_login(zs_md_api_t* mdctx, zs_login_t* login_rsp, zs_error_data_t* errdata);
static void zs_md_on_logout(zs_md_api_t* mdctx, zs_logout_t* logout_rsp, zs_error_data_t* errdata);
static void zs_md_on_subscribe(zs_md_api_t* mdctx, zs_subscribe_t* sub_rsp, int flag);
static void zs_md_on_unsubscribe(zs_md_api_t* mdctx, zs_subscribe_t* unsub_rsp, int flag);
static void zs_md_on_rtn_mktdata(zs_md_api_t* mdctx, zs_tick_t* tick);
static void zs_md_on_rtn_mktdatal2(zs_md_api_t* mdctx, zs_tickl2_t* tickl2);
static void zs_md_on_for_quote(zs_md_api_t* mdctx, void* forquote);


zs_md_api_handlers_t md_handlers = {
    zs_md_on_connect,
    zs_md_on_disconnect,
    NULL,       // on_rsp_error
    zs_md_on_login,
    zs_md_on_logout,
    zs_md_on_subscribe,
    zs_md_on_unsubscribe,
    NULL,       // on_sub_forquote
    NULL,
    zs_md_on_rtn_mktdata,
    zs_md_on_rtn_mktdatal2,
    zs_md_on_for_quote
};

//////////////////////////////////////////////////////////////////////////

#define set_zd_head_symbol(zdh, obj) \
    do {    \
        (zdh)->pSymbol = (obj)->Symbol; \
        (zdh)->SymbolLength = (uint16_t)strlen((obj)->Symbol); \
        (zdh)->ExchangeID = (obj)->ExchangeID; \
    } while (0);


static void _zs_data_free(zs_data_head_t* zdh)
{
    if (zdh)
        free(zdh);
}

static zs_data_head_t* _zs_data_create(zs_algorithm_t* algo, void* data, int size)
{
    zs_data_head_t* zdh;
    int alloc_size;

    alloc_size = ztl_align(zd_data_head_size + size, 8);

    zdh = (zs_data_head_t*)malloc(alloc_size);
    memset(zdh, 0, zd_data_head_size);
    if (data)
        ztl_memcpy(zd_data_body(zdh), data, size);

    zdh->Cleanup    = _zs_data_free;
    zdh->RefCount   = 1;
    zdh->Size       = size;

    return zdh;
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

zs_trade_api_t* zs_broker_get_tradeapi(zs_broker_t* broker, const char* apiname)
{
    if (!apiname) {
        return broker->TradeApis[0];
    }

    for (int i = 0; i < ZS_MAX_API_NUM; ++i)
    {
        if (!broker->TradeApis[i]) {
            continue;
        }

        if (strcasecmp(broker->TradeApis[i]->APIName, apiname) == 0)
            return broker->TradeApis[i];
    }
    return NULL;
}

zs_md_api_t* zs_broker_get_mdapi(zs_broker_t* broker, const char* apiname)
{
    if (!apiname) {
        return broker->MdApis[0];
    }

    for (int i = 0; i < ZS_MAX_API_NUM; ++i)
    {
        if (!broker->MdApis[i]) {
            continue;
        }

        if (strcmp(broker->MdApis[i]->APIName, apiname) == 0)
            return broker->MdApis[i];
    }
    return NULL;
}


int zs_broker_trade_load(zs_trade_api_t* tradeapi, const char* libpath)
{
    ztl_dso_handle_t* dso;
    dso = ztl_dso_load(libpath);
    if (!dso)
    {
        // ERRORID: log error
        return -1;
    }

    zs_trade_api_entry_ptr tdapi_entry_func;
    tdapi_entry_func = ztl_dso_symbol(dso, zs_trade_api_entry_name);
    if (!tdapi_entry_func)
    {
        // ERRORID: no td api entry func
        return -2;
    }

    tdapi_entry_func(tradeapi);

    tradeapi->HLib = dso;

#if 0
    tradeapi->api_version   = ztl_dso_symbol(dso, "api_version");
    tradeapi->create        = ztl_dso_symbol(dso, "create");
    tradeapi->release       = ztl_dso_symbol(dso, "release");
    tradeapi->regist        = ztl_dso_symbol(dso, "regist");
    tradeapi->connect       = ztl_dso_symbol(dso, "connect");
    tradeapi->login         = ztl_dso_symbol(dso, "login");
    tradeapi->order         = ztl_dso_symbol(dso, "order");
    tradeapi->quote_order   = ztl_dso_symbol(dso, "quote_order");
    tradeapi->cancel        = ztl_dso_symbol(dso, "cancel");
    tradeapi->query         = ztl_dso_symbol(dso, "query");
    tradeapi->request_other = ztl_dso_symbol(dso, "request_other");
#endif//0

    return 0;
}

int zs_broker_md_load(zs_md_api_t* mdapi, const char* libpath)
{
    ztl_dso_handle_t* dso;
    dso = ztl_dso_load(libpath);
    if (!dso)
    {
        // ERRORID: log error
        return -1;
    }

    zs_md_api_entry_ptr mdapi_entry_func;
    mdapi_entry_func = ztl_dso_symbol(dso, zs_md_api_entry_name);
    if (!mdapi_entry_func)
    {
        // ERRORID: no td api entry func
        return -2;
    }

    mdapi_entry_func(mdapi);

    mdapi->HLib = dso;

    return 0;
}

//////////////////////////////////////////////////////////////////////////

static void zs_td_on_connect(zs_trade_api_t* tdctx)
{
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)tdctx->UserData;
    zdh = _zs_data_create(algo, NULL, 0);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_Connected, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_disconnect(zs_trade_api_t* tdctx, int reason)
{
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)tdctx->UserData;
    zdh = _zs_data_create(algo, NULL, 0);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_Disconnected, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_error(zs_trade_api_t* tdctx, zs_error_data_t* errdata)
{
    //
}

static void zs_td_on_authenticate(zs_trade_api_t* tdctx, zs_authenticate_t* auth_rsp, zs_error_data_t* errdata)
{
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)tdctx->UserData;

    zdh = _zs_data_create(algo, auth_rsp, sizeof(zs_authenticate_t));

    rv = zs_ee_post(algo->EventEngine, ZS_DT_Auth, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_login(zs_trade_api_t* tdctx, zs_login_t* login_rsp, zs_error_data_t* errdata)
{
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)tdctx->UserData;

    zdh = _zs_data_create(algo, login_rsp, sizeof(zs_login_t));

    rv = zs_ee_post(algo->EventEngine, ZS_DT_Login, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_logout(zs_trade_api_t* tdctx, zs_logout_t* logout_rsp, zs_error_data_t* errdata)
{
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)tdctx->UserData;

    zdh = _zs_data_create(algo, logout_rsp, sizeof(zs_logout_t));

    rv = zs_ee_post(algo->EventEngine, ZS_DT_Logout, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_rtn_order(zs_trade_api_t* tdctx, zs_order_t* order)
{
    // 收到报单通知

    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_order_t*     dst_order;

    algo = (zs_algorithm_t*)tdctx->UserData;
    zdh = _zs_data_create(algo, order, sizeof(zs_order_t));
    dst_order = (zs_order_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_order);

    dst_order->Sid = zs_asset_lookup(algo->AssetFinder,
        dst_order->ExchangeID, dst_order->Symbol, zdh->SymbolLength);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_Order, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_rtn_trade(zs_trade_api_t* tdctx, zs_trade_t* trade)
{
    // 收到成交通知
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_trade_t*     dst_trade;

    algo = (zs_algorithm_t*)tdctx->UserData;
    zdh = _zs_data_create(algo, trade, sizeof(zs_trade_t));
    dst_trade = (zs_trade_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_trade);

    dst_trade->Sid = zs_asset_lookup(algo->AssetFinder,
        dst_trade->ExchangeID, dst_trade->Symbol, zdh->SymbolLength);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_Trade, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_rtn_instrument_status(zs_trade_api_t* tdctx, zs_instrument_status_t* instrument_status)
{
}

static void zs_td_on_rtn_data(zs_trade_api_t* tdctx, int dtype, void* data, int size)
{}

static void zs_td_on_qry_contract(zs_trade_api_t* tdctx, zs_contract_t* contract, zs_error_data_t* errdata, uint32_t flag)
{
    // 合约通知
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_contract_t*  dst_contract;

    algo = (zs_algorithm_t*)tdctx->UserData;
    zdh = _zs_data_create(algo, contract, sizeof(zs_contract_t));
    dst_contract = (zs_contract_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_contract);

    // dst_contract->Sid = zs_asset_lookup(algo->AssetFinder,
    //     dst_contract->ExchangeID, dst_contract->Symbol, zdh->SymbolLength);

    // post to ee
    rv = zs_ee_post(algo->EventEngine, ZS_DT_QryContract, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_qry_trading_account(zs_trade_api_t* tdctx, zs_fund_account_t* fund_account, zs_error_data_t* errdata, uint32_t flag)
{
    // 资金账户

    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)tdctx->UserData;

    zdh = _zs_data_create(algo, fund_account, sizeof(zs_fund_account_t));

    rv = zs_ee_post(algo->EventEngine, ZS_DT_QryAccount, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_qry_order(zs_trade_api_t* tdctx, zs_order_t* order, zs_error_data_t* errdata, uint32_t flag)
{
    // 报单查询应答
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_order_t*     dst_order;

    algo = (zs_algorithm_t*)tdctx->UserData;

    zdh = _zs_data_create(algo, order, sizeof(zs_order_t));
    dst_order = (zs_order_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_order);

    dst_order->Sid = zs_asset_lookup(algo->AssetFinder,
        dst_order->ExchangeID, dst_order->Symbol, zdh->SymbolLength);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_QryOrder, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_qry_trade(zs_trade_api_t* tdctx, zs_trade_t* trade, zs_error_data_t* errdata, uint32_t flag)
{
    // 成交查询应答
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_trade_t*     dst_trade;

    algo = (zs_algorithm_t*)tdctx->UserData;

    zdh = _zs_data_create(algo, trade, sizeof(zs_trade_t));
    dst_trade = (zs_trade_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_trade);

    dst_trade->Sid = zs_asset_lookup(algo->AssetFinder,
        dst_trade->ExchangeID, dst_trade->Symbol, zdh->SymbolLength);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_QryTrade, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_qry_position(zs_trade_api_t* tdctx, zs_position_t* pos, zs_error_data_t* errdata, uint32_t flag)
{
    // 持仓查询应答
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_position_t*  dst_pos;

    algo = (zs_algorithm_t*)tdctx->UserData;
    zdh = _zs_data_create(algo, pos, sizeof(zs_position_t));
    dst_pos = (zs_position_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_pos);

    dst_pos->Sid = zs_asset_lookup(algo->AssetFinder,
        dst_pos->ExchangeID, dst_pos->Symbol, zdh->SymbolLength);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_QryPosition, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_qry_position_detail(zs_trade_api_t* tdctx, zs_position_detail_t* pos_detail, zs_error_data_t* errdata, uint32_t flag)
{
    // 持仓明细查询应答
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_position_detail_t* dst_pos_detail;

    algo = (zs_algorithm_t*)tdctx->UserData;
    zdh = _zs_data_create(algo, pos_detail, sizeof(zs_position_detail_t));
    dst_pos_detail = (zs_position_detail_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_pos_detail);

    dst_pos_detail->Sid = zs_asset_lookup(algo->AssetFinder,
        dst_pos_detail->ExchangeID, dst_pos_detail->Symbol, zdh->SymbolLength);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_QryPositionDetail, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_qry_margin_rate(zs_trade_api_t* tdctx, zs_margin_rate_t* margin_rate, zs_error_data_t* errdata, uint32_t flag)
{
    // 保证金率查询应答

    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_margin_rate_t* dst_margin_rate;

    algo = (zs_algorithm_t*)tdctx->UserData;

    zdh = _zs_data_create(algo, margin_rate, sizeof(zs_margin_rate_t));
    dst_margin_rate = (zs_margin_rate_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_margin_rate);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_QryMarginRate, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_qry_commission_rate(zs_trade_api_t* tdctx, zs_commission_rate_t* comm_rate, zs_error_data_t* errdata, uint32_t flag)
{
    // 手续费率查询应答

    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_commission_rate_t* dst_comm_rate;

    algo = (zs_algorithm_t*)tdctx->UserData;

    zdh = _zs_data_create(algo, comm_rate, sizeof(zs_commission_rate_t));
    dst_comm_rate = (zs_commission_rate_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_comm_rate);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_QryCommRate, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_td_on_rsp_data(zs_trade_api_t* tdctx, int dtype, void* data, int size, zs_error_data_t* errdata, uint32_t flag)
{
}

//////////////////////////////////////////////////////////////////////////

static void zs_md_on_connect(zs_md_api_t* mdctx)
{
    fprintf(stderr, "md_on_connect\n");
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)mdctx->UserData;
    zdh = _zs_data_create(algo, NULL, 0);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_MD_Connected, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_md_on_disconnect(zs_md_api_t* mdctx, int reason)
{
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)mdctx->UserData;
    zdh = _zs_data_create(algo, NULL, 0);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_MD_Disconnected, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_md_on_login(zs_md_api_t* mdctx, zs_login_t* login_rsp, zs_error_data_t* errdata)
{
    fprintf(stderr, "md_on_login\n");

    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)mdctx->UserData;
    zdh = _zs_data_create(algo, login_rsp, sizeof(zs_login_t));

    rv = zs_ee_post(algo->EventEngine, ZS_DT_MD_Login, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_md_on_logout(zs_md_api_t* mdctx, zs_logout_t* logout_rsp, zs_error_data_t* errdata)
{
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;

    algo = (zs_algorithm_t*)mdctx->UserData;
    zdh = _zs_data_create(algo, logout_rsp, sizeof(zs_logout_t));

    rv = zs_ee_post(algo->EventEngine, ZS_DT_Logout, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_md_on_subscribe(zs_md_api_t* mdctx, zs_subscribe_t* sub_rsp, int flag)
{
    //
}

static void zs_md_on_unsubscribe(zs_md_api_t* mdctx, zs_subscribe_t* sub_rsp, int flag)
{
    //
}

static void zs_md_on_rtn_mktdata(zs_md_api_t* mdctx, zs_tick_t* tick)
{
    // level1行情通知
    int rv;
    zs_data_head_t* zdh;
    zs_algorithm_t* algo;
    zs_tick_t*      dst_tick;

    algo = (zs_algorithm_t*)mdctx->UserData;
    zdh = _zs_data_create(algo, tick, sizeof(zs_tick_t));
    dst_tick = (zs_tick_t*)zd_data_body(zdh);
    set_zd_head_symbol(zdh, dst_tick);

    dst_tick->Sid = zs_asset_lookup(algo->AssetFinder, dst_tick->ExchangeID,
        dst_tick->Symbol, zdh->SymbolLength);

    rv = zs_ee_post(algo->EventEngine, ZS_DT_MD_Tick, zdh);
    if (rv != 0)
    {
        // log error
    }
}

static void zs_md_on_rtn_mktdatal2(zs_md_api_t* mdctx, zs_tickl2_t* tickl2)
{
    //
}

static void zs_md_on_for_quote(zs_md_api_t* mdctx, void* forquote)
{
    //
}


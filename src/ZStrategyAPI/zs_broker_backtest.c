
#include <ZToolLib/ztl_map.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_broker_api.h"

#include "zs_broker_backtest.h"

#include "zs_constants_helper.h"

#include "zs_slippage.h"


/* the backtest trader apis */
zs_trade_api_t bt_tdapi = {
    "bt_tdapi",
    NULL,                   // dso handle
    NULL,                   // api user data
    NULL,                   // api instance
    0,                      // api flag
    NULL,                   // get api version
    zs_bt_trade_create,
    zs_bt_trade_release,
    zs_bt_trade_regist,
    zs_bt_trade_connect,
    NULL,
    NULL,                   // login
    NULL,                   // logout
    zs_bt_order,
    zs_bt_quote_order,
    zs_bt_cancel,
    zs_bt_query,
    NULL                    // request other
};

/* the backtest md apis */
zs_md_api_t bt_mdapi = {
    "bt_mdapi",
    NULL,                   // dso handle
    NULL,                   // api user data
    NULL,                   // api instance
    0,                      // api flag
    NULL,                   // get api version
    zs_bt_md_create,
    zs_bt_md_release,
    zs_bt_md_regist,
    zs_bt_md_connect,
    NULL,                   // login
    NULL,                   // logout
    zs_bt_md_subscribe,
    zs_bt_md_unsubscribe,
    NULL                    // request other
};


/* backtest trade api */
struct zs_bt_trade_impl_s
{
    zs_conf_account_t           ApiConf;
    void*                       TdCtx;
    zs_trade_api_handlers_t*    TdHandlers;
    zs_slippage_t*              Slippage;   // like a network connection
};

struct zs_bt_md_impl_s
{
    zs_conf_account_t       ApiConf;
    void*                   MdCtx;
    zs_md_api_handlers_t*   MdHandlers;
    ztl_set_t*              SymbolSet;      // sid set
    zs_algorithm_t*         Algorithm;
};

typedef struct zs_bt_trade_impl_s zs_bt_trade_impl_t;
typedef struct zs_bt_md_impl_s zs_bt_md_impl_t;


// create backtest trade api instance
void* zs_bt_trade_create(const char* str, int reserve)
{
    zs_bt_trade_impl_t* td_instance;

    td_instance = (zs_bt_trade_impl_t*)calloc(1, sizeof(zs_bt_trade_impl_t));

    td_instance->Slippage = NULL;

    return td_instance;
}

// release api instance
void zs_bt_trade_release(void* api_instance)
{
    zs_bt_trade_impl_t* td_instance;
    td_instance = (zs_bt_trade_impl_t*)api_instance;
    if (td_instance)
    {
        free(td_instance);
    }
}

int zs_bt_trade_regist(void* api_instance, zs_trade_api_handlers_t* td_handlers,
    void* tdctx, const zs_conf_account_t* conf)
{
    zs_bt_trade_impl_t* td_instance;
    td_instance = (zs_bt_trade_impl_t*)api_instance;

    td_instance->TdHandlers = td_handlers;
    td_instance->ApiConf = *conf;
    td_instance->TdCtx = tdctx;

    return 0;
}

int zs_bt_trade_connect(void* api_instance, void* addr)
{
    zs_bt_trade_impl_t* td_instance;
    td_instance = (zs_bt_trade_impl_t*)api_instance;

    // how??
    td_instance->Slippage = addr;

    return 0;
}

int zs_bt_order(void* api_instance, zs_order_req_t* order_req)
{
    zs_bt_trade_impl_t* td_instance;
    td_instance = (zs_bt_trade_impl_t*)api_instance;

    return zs_slippage_order(td_instance->Slippage, order_req);
}

int zs_bt_quote_order(void* api_instance, zs_quote_order_req_t* order_req)
{
    zs_bt_trade_impl_t* td_instance;
    td_instance = (zs_bt_trade_impl_t*)api_instance;

    return zs_slippage_quote_order(td_instance->Slippage, order_req);
}

int zs_bt_cancel(void* api_instance, zs_cancel_req_t* cancel_req)
{
    zs_bt_trade_impl_t* td_instance;
    td_instance = (zs_bt_trade_impl_t*)api_instance;

    return zs_slippage_cancel(td_instance->Slippage, cancel_req);
}

int zs_bt_query(void* api_instance, ZSApiQueryCategory category, void* data, int size)
{
    // no need support when backtest
    return -1;
}


// create backtest md api instance
void* zs_bt_md_create(const char* str, int reserve)
{
    zs_bt_md_impl_t* md_instrance;
    md_instrance = (zs_bt_md_impl_t*)calloc(1, sizeof(zs_bt_md_impl_t));
    return md_instrance;
}

// release api instance
void zs_bt_md_release(void* api_instance)
{
    zs_bt_md_impl_t* md_instrance;
    md_instrance = (zs_bt_md_impl_t*)api_instance;

    if (md_instrance)
    {
        free(md_instrance);
    }
}

int zs_bt_md_regist(void* api_instance, zs_md_api_handlers_t* md_handlers,
    void* mdctx, const zs_conf_account_t* conf)
{
    zs_bt_md_impl_t* md_instrance;
    md_instrance = (zs_bt_md_impl_t*)api_instance;

    md_instrance->MdHandlers = md_handlers;
    md_instrance->ApiConf = *conf;
    md_instrance->MdCtx = mdctx;

    return 0;
}

int zs_bt_md_connect(void* api_instance, void* addr)
{
    zs_bt_md_impl_t* md_instrance;
    md_instrance = (zs_bt_md_impl_t*)api_instance;

    // do nothing
    (void)md_instrance;

    return 0;
}

int zs_bt_md_login(void* api_instance)
{
    return -1;
}

int zs_bt_md_subscribe(void* api_instance, zs_subscribe_t* sub_reqs[], int count)
{
    zs_bt_md_impl_t* md_instrance;
    md_instrance = (zs_bt_md_impl_t*)api_instance;

    // record what instruments subscribe
    char* instr;
    zs_sid_t sid;
    for (int i = 0; i < count; ++i)
    {
        int exchangeid = zs_convert_exchange_name(sub_reqs[i]->Exchange);
        instr = sub_reqs[i]->Symbol;
        sid = zs_asset_lookup(md_instrance->Algorithm->AssetFinder, 
            exchangeid, instr, (int)strlen(instr));
        ztl_set_add(md_instrance->SymbolSet, sid);
    }

    return 0;
}

int zs_bt_md_unsubscribe(void* api_instance, zs_subscribe_t* unsub_reqs[], int count)
{
    zs_bt_md_impl_t* md_instrance;
    md_instrance = (zs_bt_md_impl_t*)api_instance;

    // record what instruments subscribe
    char* instr;
    zs_sid_t sid;
    for (int i = 0; i < count; ++i)
    {
        int exchangeid = zs_convert_exchange_name(unsub_reqs[i]->Exchange);
        instr = unsub_reqs[i]->Symbol;
        sid = zs_asset_lookup(md_instrance->Algorithm->AssetFinder,
            exchangeid, instr, (int)strlen(instr));
        ztl_set_del(md_instrance->SymbolSet, sid);
    }

    return 0;
}



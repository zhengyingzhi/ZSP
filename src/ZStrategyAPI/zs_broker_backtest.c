﻿
#include <ZToolLib/ztl_map.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_broker_api.h"

#include "zs_broker_backtest.h"

#include "zs_slippage.h"


/* trader apis */
zs_trade_api_t bt_tdapi = {
    "bt_tdapi",
    NULL,           // api instance
    NULL,           // api user data
    zs_bt_trade_create,
    zs_bt_trade_release,
    zs_bt_trade_regist,
    zs_bt_trade_connect,
    NULL,           // login
    NULL,           // logout
    zs_bt_order,
    zs_bt_quote_order,
    zs_bt_cancel,
    zs_bt_query,
    NULL            // request other
};

/* md apis */
zs_md_api_t bt_mdapi = {
    "bt_mdapi",
    NULL,           // api instance
    NULL,           // api user data
    zs_bt_md_create,
    zs_bt_md_release,
    zs_bt_md_regist,
    zs_bt_md_connect,
    NULL,           // login
    NULL,           // logout
    zs_bt_md_subscribe,
    zs_bt_md_unsubscribe,
    NULL            // request other
};


/* backtest trade api */
struct zs_bt_trade_api_s
{
    zs_broker_conf_t         ApiConf;
    zs_trade_api_handlers_t *TdHandlers;
    zs_slippage_t*          Slippage;   // like a network connection
};

struct zs_bt_md_api_s
{
    zs_broker_conf_t        ApiConf;
    zs_md_api_handlers_t*   MdHandlers;
    ztl_set_t*              SymbolSet;  // sid set
    zs_algorithm_t*         Algorithm;
};

typedef struct zs_bt_trade_api_s zs_bt_trade_api_t;
typedef struct zs_bt_md_api_s zs_bt_md_api_t;


// create backtest trade api instance
void* zs_bt_trade_create(const char* str, int reserve)
{
    zs_bt_trade_api_t* tdapi;

    tdapi = (zs_bt_trade_api_t*)calloc(1, sizeof(zs_bt_trade_api_t));

    tdapi->Slippage = NULL;

    return tdapi;
}

// release api instance
void zs_bt_trade_release(void* api_instance)
{
    zs_bt_trade_api_t* tdapi;
    tdapi = (zs_bt_trade_api_t*)api_instance;
    if (tdapi)
    {
        free(tdapi);
    }
}

int zs_bt_trade_regist(void* api_instance, zs_trade_api_handlers_t* tdHandlers,
    void* tdCtx, const zs_broker_conf_t* apiConf)
{
    zs_bt_trade_api_t* tdapi;
    tdapi = (zs_bt_trade_api_t*)api_instance;

    tdapi->TdHandlers = tdHandlers;
    tdapi->ApiConf = *apiConf;

    return 0;
}

int zs_bt_trade_connect(void* api_instance, void* addr)
{
    zs_bt_trade_api_t* tdapi;
    tdapi = (zs_bt_trade_api_t*)api_instance;

    // how??
    tdapi->Slippage = addr;

    return 0;
}

int zs_bt_order(void* api_instance, zs_order_req_t* order_req)
{
    zs_bt_trade_api_t* tdapi;
    tdapi = (zs_bt_trade_api_t*)api_instance;

    return zs_slippage_order(tdapi->Slippage, order_req);
}

int zs_bt_quote_order(void* api_instance, zs_quote_order_req_t* orderReq)
{
    zs_bt_trade_api_t* tdapi;
    tdapi = (zs_bt_trade_api_t*)api_instance;

    return zs_slippage_quote_order(tdapi->Slippage, orderReq);
}

int zs_bt_cancel(void* api_instance, zs_cancel_req_t* cancelReq)
{
    zs_bt_trade_api_t* tdapi;
    tdapi = (zs_bt_trade_api_t*)api_instance;

    return zs_slippage_cancel(tdapi->Slippage, cancelReq);
}

int zs_bt_query(void* api_instance, ZSApiQueryCategory category, void* data, int size)
{
    // no need support when backtest
    return -1;
}


// create backtest md api instance
void* zs_bt_md_create(const char* str, int reserve)
{
    zs_bt_md_api_t* mdapi;
    mdapi = (zs_bt_md_api_t*)calloc(1, sizeof(zs_bt_md_api_t));
    return mdapi;
}

// release api instance
void zs_bt_md_release(void* api_instance)
{
    zs_bt_md_api_t* mdapi;
    mdapi = (zs_bt_md_api_t*)api_instance;

    if (mdapi)
    {
        free(mdapi);
    }
}

int zs_bt_md_regist(void* api_instance, zs_md_api_handlers_t* md_handlers,
    void* mdctx, const zs_broker_conf_t* conf)
{
    zs_bt_md_api_t* mdapi;
    mdapi = (zs_bt_md_api_t*)api_instance;

    mdapi->MdHandlers = md_handlers;
    mdapi->ApiConf = *conf;

    return 0;
}

int zs_bt_md_connect(void* api_instance, void* addr)
{
    zs_bt_md_api_t* mdapi;
    mdapi = (zs_bt_md_api_t*)api_instance;

    // do nothing
    (void)mdapi;

    return 0;
}

int zs_bt_md_login(void* api_instance)
{
    return -1;
}

int zs_bt_md_subscribe(void* api_instance, zs_subscribe_t* sub_reqs[], int count)
{
    zs_bt_md_api_t* mdapi;
    mdapi = (zs_bt_md_api_t*)api_instance;

    // record what instruments subscribe
    char* instr;
    zs_sid_t sid;
    for (int i = 0; i < count; ++i)
    {
        instr = sub_reqs[i]->Symbol;
        sid = zs_asset_lookup(mdapi->Algorithm->AssetFinder, instr, (int)strlen(instr));
        ztl_set_add(mdapi->SymbolSet, sid);
    }

    return 0;
}

int zs_bt_md_unsubscribe(void* api_instance, zs_subscribe_t* unsub_reqs[], int count)
{
    zs_bt_md_api_t* mdapi;
    mdapi = (zs_bt_md_api_t*)api_instance;

    // record what instruments subscribe
    char* instr;
    zs_sid_t sid;
    for (int i = 0; i < count; ++i)
    {
        instr = unsub_reqs[i]->Symbol;
        sid = zs_asset_lookup(mdapi->Algorithm->AssetFinder, instr, (int)strlen(instr));
        ztl_set_del(mdapi->SymbolSet, sid);
    }

    return 0;
}



#include <ZToolLib/ztl_map.h>

#include "zs_algorithm.h"
#include "zs_assets.h"
#include "zs_broker_api.h"
#include "zs_broker_backtest.h"
#include "zs_constants_helper.h"
#include "zs_slippage.h"

#ifdef _MSC_VER
#include<process.h>
#include<windows.h>
#else
#include <pthread.h>
#endif//_MSC_VER


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
    // zs_algorithm_t*         Algorithm;

    void*                   thr;
    volatile int32_t        running;
};

typedef struct zs_bt_trade_impl_s zs_bt_trade_impl_t;
typedef struct zs_bt_md_impl_s zs_bt_md_impl_t;


// create backtest trade api instance
void* zs_bt_trade_create(const char* str, int reserve)
{
    zs_bt_trade_impl_t* instance;

    instance = (zs_bt_trade_impl_t*)calloc(1, sizeof(zs_bt_trade_impl_t));

    instance->Slippage = NULL;

    return instance;
}

// release api instance
void zs_bt_trade_release(zs_bt_trade_impl_t* instance)
{
    if (instance)
    {
        free(instance);
    }
}

int zs_bt_trade_regist(zs_bt_trade_impl_t* instance, zs_trade_api_handlers_t* td_handlers,
    void* tdctx, const zs_conf_account_t* conf)
{
    instance->TdHandlers = td_handlers;
    if (conf)
        instance->ApiConf = *conf;
    instance->TdCtx = tdctx;

    return 0;
}

int zs_bt_trade_connect(zs_bt_trade_impl_t* instance, void* addr)
{
    // how??
    instance->Slippage = (zs_slippage_t*)addr;

    return 0;
}

int zs_bt_order(zs_bt_trade_impl_t* instance, zs_order_req_t* order_req)
{
    fprintf(stderr, "zs_bt_order symbol:%s,qty:%d,px:%2.lf,dir:%d,offset:%d\n",
        order_req->Symbol, order_req->OrderQty, order_req->OrderPrice, order_req->Direction, order_req->OffsetFlag);

    if (instance->Slippage) {
        return zs_slippage_order(instance->Slippage, order_req);
    }
    else
    {
        static uint32_t order_id = 1;

        // 
        extern void zs_convert_order_req(zs_order_t*, const zs_order_req_t*);
        sprintf(order_req->OrderID, "%d", order_id++);

        zs_order_t order = { 0 };
        zs_convert_order_req(&order, order_req);
        order.OrderStatus = ZS_OS_Pending;
        strcpy(order.OrderSysID, order_req->OrderID);       // fake

        instance->TdHandlers->on_rtn_order(instance->TdCtx, &order);

        // 
        order.OrderStatus = ZS_OS_PartFilled;
        order.FilledQty = 1;
        if (instance->TdHandlers->on_rtn_order)
            instance->TdHandlers->on_rtn_order(instance->TdCtx, &order);

        zs_trade_t trade = { 0 };
        strcpy(trade.Symbol, order.Symbol);
        strcpy(trade.AccountID, order.AccountID);
        strcpy(trade.UserID, order.UserID);
        strcpy(trade.OrderID, order.OrderID);
        strcpy(trade.OrderSysID, order.OrderID);       // fake
        strcpy(trade.TradeID, order.OrderID);
        trade.FrontID = order.FrontID;
        trade.SessionID = order.SessionID;
        trade.ExchangeID = order.ExchangeID;
        trade.Sid = order.Sid;
        trade.Direction = order.Direction;
        trade.OffsetFlag = order.OffsetFlag;
        trade.Volume = 1;
        trade.Price = order.OrderPrice;
        if (instance->TdHandlers->on_rtn_trade)
            instance->TdHandlers->on_rtn_trade(instance->TdCtx, &trade);
    }
    return 0;
}

int zs_bt_quote_order(zs_bt_trade_impl_t* instance, zs_quote_order_req_t* order_req)
{
    if (instance->Slippage) {
        return zs_slippage_quote_order(instance->Slippage, order_req);
    }
    return 0;
}

int zs_bt_cancel(zs_bt_trade_impl_t* instance, zs_cancel_req_t* cancel_req)
{
    if (instance->Slippage) {
        return zs_slippage_cancel(instance->Slippage, cancel_req);
    }
    return 0;
}

int zs_bt_query(zs_bt_trade_impl_t* instance, ZSApiQueryCategory category, void* data, int size)
{
    // no need support when backtest
    return -1;
}


//////////////////////////////////////////////////////////////////////////
// debug test
static unsigned int __stdcall bt_md_sim_thread(void* data)
{
    fprintf(stderr, "bt_md_sim_thread running\n");
    zs_bt_md_impl_t* md_instrance;
    md_instrance = (zs_bt_md_impl_t*)data;
    int index = 0;

    zs_tick_t tick = { 0 };

    while (md_instrance->running)
    {
        index += 1;
        Sleep(3000);

        // publish one tick
        strcpy(tick.Symbol, "rb1910");
        tick.LastPrice = 2580;
        tick.Volume = 123 + index;
        tick.Turnover = 3456765 + index * 2000;
        tick.UpdateTime = 142300500 + index * 10000;
        tick.ExchangeID = ZS_EI_SHFE;

        if (md_instrance->MdHandlers)
            md_instrance->MdHandlers->on_rtn_mktdata(md_instrance->MdCtx, &tick);
    }
    return 0;
}


// create backtest md api instance
void* zs_bt_md_create(const char* str, int reserve)
{
    zs_bt_md_impl_t* md_instrance;
    md_instrance = (zs_bt_md_impl_t*)calloc(1, sizeof(zs_bt_md_impl_t));

    md_instrance->SymbolSet = ztl_set_create(32);

    return md_instrance;
}

// release api instance
void zs_bt_md_release(zs_bt_md_impl_t* md_instrance)
{
    if (md_instrance)
    {
        free(md_instrance);
    }
}

int zs_bt_md_regist(zs_bt_md_impl_t* md_instrance, zs_md_api_handlers_t* md_handlers,
    void* mdctx, const zs_conf_account_t* conf)
{
    md_instrance->MdHandlers = md_handlers;
    if (conf)
        md_instrance->ApiConf = *conf;
    md_instrance->MdCtx = mdctx;

    return 0;
}

int zs_bt_md_connect(zs_bt_md_impl_t* md_instrance, void* addr)
{
    // do nothing
    (void)md_instrance;

#if 0
    if (!md_instrance->running)
    {
        md_instrance->running = 1;
        md_instrance->thr = (HANDLE)_beginthreadex(NULL, 0, bt_md_sim_thread, md_instrance, 0, NULL);
    }
#endif//0

    return 0;
}

int zs_bt_md_login(zs_bt_md_impl_t* md_instrance)
{
    return -1;
}

int zs_bt_md_subscribe(zs_bt_md_impl_t* md_instrance, zs_subscribe_t* sub_reqs[], int count)
{
    // record what instruments subscribe

    for (int i = 0; i < count; ++i)
    {
        ztl_set_add(md_instrance->SymbolSet, (uint64_t)sub_reqs[i]->Sid);
    }

    return 0;
}

int zs_bt_md_unsubscribe(zs_bt_md_impl_t* md_instrance, zs_subscribe_t* unsub_reqs[], int count)
{
    // record what instruments subscribe

    for (int i = 0; i < count; ++i)
    {
        ztl_set_del(md_instrance->SymbolSet, (uint64_t)unsub_reqs[i]->Sid);
    }

    return 0;
}


//////////////////////////////////////////////////////////////////////////
/* the backtest trader apis */
zs_trade_api_t bt_tdapi = {
    ZS_BACKTEST_APINAME,
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
    ZS_BACKTEST_APINAME,
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


int zs_bt_trade_api_entry(zs_trade_api_t* tdapi)
{
    *tdapi = bt_tdapi;
    return 0;
}

int zs_bt_md_api_entry(zs_md_api_t* mdapi)
{
    *mdapi = bt_mdapi;
    return 0;
}


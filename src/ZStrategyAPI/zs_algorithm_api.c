
#include "zs_api_object.h"

#include "zs_algorithm.h"

#include "zs_algorithm_api.h"

#include "zs_assets.h"

#include "zs_blotter.h"

#include "zs_broker_api.h"

#include "zs_broker_entry.h"

#include "zs_core.h"

#include "zs_risk_control.h"


/* the implemented api for strategy developer */


int zs_order(zs_algorithm_t* algo, const char* accountid, 
    zs_sid_t sid, int order_qty, double order_price, int stop_order)
{
    int rv;
    zs_contract_t*  contract;
    // zs_account_t*   account;
    zs_order_req_t  order_req = { 0 };

    contract = zs_asset_find_by_sid(algo->AssetFinder, sid);
    if (!contract) {
        return -2;
    }

    if (!accountid) {
        // account = zs_blotter_manager_get(&algo->BlotterMgr, NULL)->pAccount;
        // accountid = account->accountid;
    }

    strncpy(order_req.AccountID, accountid, sizeof(order_req.AccountID) - 1);
    strncpy(order_req.Symbol, contract->Symbol, sizeof(order_req.Symbol) - 1);
    order_req.Sid = sid;
    order_req.OrderQty = abs(order_qty);
    order_req.OrderPrice = order_price;
    order_req.OrderType = order_price > 0.000001 ? ZS_OT_Limit : ZS_OT_Market;
    order_req.Direction = order_qty > 0 ? ZS_D_Long : ZS_D_Short;

    rv = zs_order_ex(algo, &order_req, stop_order);

    return rv;
}

int zs_order_ex(zs_algorithm_t* algo, zs_order_req_t* order_req, int stop_order)
{
    int rv;

    // 开仓：计算资金，风控等
    // 平仓：检验持仓等

    //zs_trade_api_t* td_api;
    //td_api = zs_broker_get_tradeapi(algo->Broker, api_name);
    //if (td_api == NULL)
    //{
    //    return -1;
    //}

    // 开仓：计算资金，风控等
    // 平仓：检验持仓等

    zs_blotter_t* blotter;
    blotter = zs_blotter_manager_get(&algo->BlotterMgr, order_req->AccountID);

    rv = zs_blotter_order(blotter, order_req);

    return rv;
}

int zs_order_target(zs_algorithm_t* algo, const char* accountid, zs_sid_t sid, int target)
{
    // 获取持仓，生成一个或多个订单
    return 0;
}

int zs_order_target_value(zs_algorithm_t* algo, const char* accountid, 
    zs_sid_t sid, double limitpx, double target)
{
    // 获取行情，计算下单量
    
    return 0;
}

int zs_order_target_percent(zs_algorithm_t* algo, const char* accountid, 
    zs_sid_t sid, double limitpx, float target)
{
    // 获取资金，计算下单量
    return 0;
}

int zs_order_cancel(zs_algorithm_t* algo, const char* orderid)
{
    int rv;
    const char* api_name = NULL;
    zs_order_t* order;
    zs_trade_api_t* td_api;
    zs_blotter_t* blotter;

    blotter = zs_blotter_manager_get(&algo->BlotterMgr, NULL);
    // order = zs_get_order_byid(blotter, orderid);
    order = NULL;
    if (!orderid)
    {
        return -2;
    }

    td_api = zs_broker_get_tradeapi(algo->Broker, api_name);
    if (td_api == NULL)
    {
        return -3;
    }

    zs_cancel_req_t cancel_req = { 0 };
    strcpy(cancel_req.Symbol, order->Symbol);
    cancel_req.ExchangeID = order->ExchangeID;
    //strcpy(cancel_req.accountid, blotter->pAccount->accountid);
    strcpy(cancel_req.OrderSysID, order->OrderSysID);
    strcpy(cancel_req.OrderID, orderid);

    rv = td_api->cancel(td_api->ApiInstance, &cancel_req);
    if (rv != 0)
    {
        // 
    }

    return rv;
}

int zs_order_cancel_byorder(zs_algorithm_t* algo, zs_order_t* order)
{
    return 0;
}

int zs_order_cancel_batch(zs_algorithm_t* algo, const char* accountid, 
    const char* symbol, int directionFlag)
{
    /* directionFlag==Long -->> cancel buy
     * directionFlag==Short -->> cancel sell
     * directionFlag==Long+Short -->> cancel all
     */
    return 0;
}

int zs_subscribe(zs_algorithm_t* algo, const char* symbol, const char* exchange)
{
    return 0;
}

int zs_get_open_orders(zs_algorithm_t* algo, const char* accountid, 
    const char* symbol,  zs_order_t* orders[], int ordsize)
{
    return 0;
}


int zs_set_commission(zs_algorithm_t* algo)
{
    return 0;
}

int zs_set_margin(zs_algorithm_t* algo)
{
    return 0;
}

int zs_set_long_only(zs_algorithm_t* algo)
{
    // only available for equity
    return 0;
}

int zs_set_max_leverage(zs_algorithm_t* algo, float leverage)
{
    return 0;
}

zs_contract_t* zs_get_contract(zs_algorithm_t* algo, int exchangeid, const char* symbol)
{
    return zs_asset_find(algo->AssetFinder, exchangeid, symbol, (int)strlen(symbol));
}

#if 0
zs_account_t* zs_get_account(zs_algorithm_t* algo, const char* accountid)
{
    zs_account_t* account;
    zs_blotter_t* blotter;

    blotter = zs_blotter_manager_get(&algo->BlotterMgr, accountid);
    account = blotter->pAccount;

    return account;
}
#endif

zs_portfolio_t* zs_get_portfolio(zs_algorithm_t* algo, const char* accountid)
{
    zs_portfolio_t* portfolio;
    zs_blotter_t* blotter;

    blotter = zs_blotter_manager_get(&algo->BlotterMgr, accountid);
    portfolio = blotter->pPortfolio;

    return portfolio;
}

int zs_handle_splits(zs_algorithm_t* algo, const char* symbol, double adjust_factor)
{
    // only available for equity
    return 0;
}


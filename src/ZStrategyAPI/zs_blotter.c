#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_palloc.h>
#include <ZToolLib/ztl_memcpy.h>
#include <ZToolLib/ztl_utils.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_blotter.h"

#include "zs_constants_helper.h"

#include "zs_data_portal.h"

#include "zs_position.h"

#include "zs_risk_control.h"


typedef struct  
{
    uint16_t    ExchangeID;
    uint16_t    Length;
    char*       pOrderSysID;
}ZSOrderSysKey;

typedef struct
{
    int32_t     SessionID;
    uint16_t    FrontID;
    uint16_t    Length;
    char*       pOrderID;
}ZSOrderKey;

static uint64_t _zs_order_keyhash(const void *key) {
    ZSOrderKey* skey = (ZSOrderKey*)key;
    return dictGenHashFunction((unsigned char*)skey->pOrderID, skey->Length);
}

static void* _zs_order_keydup(void* priv, const void* key) {
    zs_blotter_t* blotter = (zs_blotter_t*)priv;
    ZSOrderKey* skey = (ZSOrderKey*)key;
    ZSOrderKey* dup_key = (ZSOrderKey*)ztl_palloc(blotter->Pool, ztl_align(sizeof(ZSOrderKey) + skey->Length + 1, 4));
    dup_key->SessionID = skey->SessionID;
    dup_key->Length = skey->Length;
    dup_key->pOrderID = (char*)(dup_key + 1);
    ztl_memcpy(dup_key->pOrderID, skey->pOrderID, skey->Length);  // could be use a faster copy
    return dup_key;
}

static void* _zs_order_valdup(void* priv, const void* obj) {
    zs_blotter_t* blotter = (zs_blotter_t*)priv;
    zs_order_t* dupord = ztl_palloc(blotter->Pool, sizeof(zs_order_t));
    ztl_memcpy(dupord, obj, sizeof(zs_order_t));
    return dupord;
}

static int _zs_order_keycmp(void* priv, const void* s1, const void* s2) {
    (void)priv;
    ZSOrderKey* k1 = (ZSOrderKey*)s1;
    ZSOrderKey* k2 = (ZSOrderKey*)s2;
    return (k1->SessionID == k2->SessionID) \
        && (k1->FrontID == k2->FrontID) \
        && memcmp(k1->pOrderID, k2->pOrderID, k2->Length) == 0;
}

static dictType orderHashDictType = {
    _zs_order_keyhash,
    _zs_order_keydup,
    _zs_order_valdup,
    _zs_order_keycmp,
    NULL,
    NULL
};


//////////////////////////////////////////////////////////////////////////

zs_blotter_t* zs_blotter_create(zs_algorithm_t* algo, const char* accountid)
{
    ztl_pool_t*         pool;
    zs_blotter_t*       blotter;
    zs_conf_account_t*  account_conf;

    account_conf = zs_configs_find_account(algo->Params, accountid);
    if (!account_conf)
    {
        // ERRORID: not find the account
        return NULL;
    }

    pool = algo->Pool;
    blotter = (zs_blotter_t*)ztl_pcalloc(pool, sizeof(zs_blotter_t));
    blotter->Pool = pool;

    blotter->Algorithm = algo;

    blotter->OrderDict = dictCreate(&orderHashDictType, blotter);
    blotter->WorkOrderList = zs_orderlist_create();

    ztl_array_init(&blotter->Trades, pool, 8192, sizeof(void*));

    blotter->Positions = ztl_map_create(64);

    blotter->Account = zs_account_create(blotter->Pool);
    blotter->pPortfolio = (zs_portfolio_t*)ztl_pcalloc(pool, sizeof(zs_portfolio_t));

    blotter->Commission = zs_commission_create(blotter->Algorithm);

    // demo debug
    zs_fund_account_t* fund_account = &blotter->Account->FundAccount;
    strcpy(fund_account->AccountID, account_conf->AccountID);
    fund_account->Available = 1000000;
    fund_account->Balance = 1000000;

    blotter->RiskControl = algo->RiskControl;

    //blotter->TradeApi = zs_broker_get_tradeapi()

    blotter->handle_order_submit = zs_handle_order_submit;
    blotter->handle_order_returned = zs_handle_order_returned;
    blotter->handle_order_trade = zs_handle_order_trade;

    return blotter;
}

void zs_blotter_release(zs_blotter_t* blotter)
{
    if (!blotter) {
        return;
    }

    if (blotter->Account) {
        zs_account_release(blotter->Account);
        blotter->Account = NULL;
    }

    if (blotter->OrderDict) {
        dictRelease(blotter->OrderDict);
        blotter->OrderDict = NULL;
    }

    if (blotter->WorkOrderList) {
        zs_orderlist_create(blotter->WorkOrderList);
        blotter->WorkOrderList = NULL;
    }

    if (blotter->Positions) {
        ztl_map_release(blotter->Positions);
        blotter->Positions = NULL;
    }

    if (blotter->Commission) {
        zs_commission_release(blotter->Commission);
        blotter->Commission = NULL;
    }

}

int zs_blotter_order(zs_blotter_t* blotter, zs_order_req_t* order_req)
{
    int rv;
    zs_trade_api_t* tdapi;

    rv = zs_risk_control_check(blotter->Algorithm->RiskControl, order_req);
    if (rv != 0)
    {
        return rv;
    }

    rv = zs_handle_order_submit(blotter, order_req);
    if (rv != 0) {
        return rv;
    }

    tdapi = blotter->TradeApi;
    rv = tdapi->order(tdapi->ApiInstance, order_req);
    if (rv != 0)
    {
        // send order failed
        zs_order_t order = { 0 };
        strcpy(order.AccountID, order_req->AccountID);
        strcpy(order.Symbol, order_req->Symbol);
        order.ExchangeID = order_req->ExchangeID;
        order.Sid = order_req->Sid;
        order.OrderStatus = ZS_OS_Rejected;
        order.Direction = order_req->Direction;
        order.OffsetFlag = order_req->OffsetFlag;
        // other fields...

        zs_handle_order_returned(blotter, &order);
        return rv;
    }

    // save this order 
    zs_blotter_save_order(blotter, order_req);

    return rv;
}

int zs_blotter_quote_order(zs_blotter_t* blotter, zs_quote_order_req_t* quote_req)
{
    return -1;
}

int zs_blotter_cancel(zs_blotter_t* blotter, zs_cancel_req_t* cancel_req)
{
    // 撤单请求
    int rv;
    zs_trade_api_t* tdapi;

    tdapi = blotter->TradeApi;
    rv = tdapi->cancel(tdapi->ApiInstance, cancel_req);
    if (rv != 0)
    {
        // 
    }
    return rv;
}

int zs_blotter_save_order(zs_blotter_t* blotter, zs_order_req_t* order_req)
{
    // 保存订单到 OrderDict 和 WorkOrderDict
    // TODO: conv order_req as order object

    zs_order_t      order;
    ZSOrderKey      key;

    order.SessionID = order_req->SessionID;
    order.FrontID = order_req->FrontID;
    strcpy(order.OrderID, order_req->OrderID);
    strcpy(order.UserID, order_req->UserID);

    key.SessionID = order_req->SessionID;
    key.FrontID = order_req->FrontID;
    key.Length = (int)strlen(order_req->OrderID);

    dictAdd(blotter->OrderDict, &key, &order);

    // zs_orderlist_append(blotter->WorkOrderList, &order);

    return 0;
}


zs_order_t* zs_get_order_by_sysid(zs_blotter_t* blotter, ZSExchangeID exchange_id, const char* order_sysid)
{
    zs_order_t* order = NULL;
    order = zs_order_find_by_sysid(blotter->WorkOrderList, exchange_id, order_sysid);
    return order;
}

zs_order_t* zs_get_order_by_id(zs_blotter_t* blotter, int32_t frontid, int32_t sessionid, const char* orderid)
{
    zs_order_t* order = NULL;
    order = zs_order_find(blotter->WorkOrderList, frontid, sessionid, orderid);
    return order;
}

zs_position_engine_t* zs_get_position_engine(zs_blotter_t* blotter, zs_sid_t sid)
{
    zs_position_engine_t* pos;
    pos = ztl_map_find(blotter->Positions, sid);
    return pos;
}


//////////////////////////////////////////////////////////////////////////
// 订单事件
int zs_handle_order_submit(zs_blotter_t* blotter, zs_order_req_t* order_req)
{
    // 处理订单提交请求:
    // 1. 风控
    // 2. 开仓-冻结资金, 平仓-冻结持仓
    // 3. 手续费
    int                     rv;
    zs_account_t*           account;
    zs_position_engine_t*   position;
    zs_contract_t*          contract;

    if (order_req->Contract) {
        contract = (zs_contract_t*)order_req->Contract;
    }
    else {
        contract = zs_asset_find_by_sid(blotter->Algorithm->AssetFinder, order_req->Sid);
    }

    if (!contract) {
        // ERRORID: not find the contract data
        return -1;
    }

    account = blotter->Account;

    // 自成交检测
    // blotter->WorkWorkOrderList

    // 持仓
    if (order_req->OffsetFlag != ZS_OF_Open)
    {
        position = zs_get_position_engine(blotter, order_req->Sid);
        if (position)
        {
            rv = position->handle_order_req(position, order_req);
            if (rv != 0) {
                return rv;
            }
        }
    }

    // 资金
    rv = zs_account_on_order_req(account, order_req, contract);
    if (rv != 0) {
        return rv;
    }

    return 0;
}

int zs_handle_quote_order_submit(zs_blotter_t* blotter,
    zs_quote_order_req_t* quote_req)
{
    return -1;
}

int zs_handle_order_returned(zs_blotter_t* blotter, zs_order_t* order)
{
    zs_order_t*     lorder;
    zs_contract_t*  contract;
    ZSOrderStatus   order_status;
    ZSOrderKey      key;
    dictEntry*      entry;

    key.SessionID = order->SessionID;
    key.FrontID = order->FrontID;
    key.Length = (int)strlen(order->OrderID);
    key.pOrderID = order->OrderID;

    contract = zs_asset_find_by_sid(blotter->Algorithm->AssetFinder, order->Sid);

    // 查找本地委托并更新，若为挂单，则更新到workorders中，否则从workorders中删除

    order_status = order->OrderStatus;
    entry = dictFind(blotter->OrderDict, &key);
    if (entry)
    {
        zs_order_t* old_order;
        old_order = (zs_order_t*)entry->v.val;
        if (is_finished_status(order_status))
        // if (status == ZS_OS_Filled || status == ZS_OS_Canceld || status == ZS_OS_Rejected)
        {
            // 重复订单
            if (order_status == old_order->OrderStatus) {
                return 0;
            }
        }

        // 更新订单状态
    }
    else
    {
        dictAdd(blotter->OrderDict, &key, order);
    }

    // working order
    lorder = zs_get_order_by_sysid(blotter, order->ExchangeID, order->OrderSysID);
    if (!lorder)
    {
        return -1;
    }

    strcpy(lorder->OrderSysID, order->OrderSysID);
    lorder->FilledQty = order->FilledQty;
    lorder->OrderStatus = order->OrderStatus;
    lorder->OrderTime = order->OrderTime;
    lorder->CancelTime = order->CancelTime;

    zs_account_on_order_rtn(blotter->Account, lorder, contract);

    if (is_finished_status(order->OrderStatus))
    {
        zs_position_engine_t* position;
        position = zs_get_position_engine(blotter, order->Sid);
        if (position)
        {
            position->handle_order_rtn(position, order);
        }
    }

    return 0;
}

int zs_handle_order_trade(zs_blotter_t* blotter, zs_trade_t* trade)
{
    // 开仓单成交：调整持仓，重新计算占用资金，计算持仓成本
    // 平仓单成交：解冻持仓，回笼资金
    zs_order_t* lorder;
    lorder = zs_get_order_by_sysid(blotter, trade->ExchangeID, trade->OrderSysID);
    if (!lorder)
    {
        return -1;
    }

    lorder->FilledQty += trade->Volume;
    if (lorder->FilledQty == lorder->OrderQty)
        lorder->OrderStatus = ZS_OS_Filled;
    else
        lorder->OrderStatus = ZS_OS_PartFilled;

    zs_position_engine_t*  position;
    position = zs_get_position_engine(blotter, trade->Sid);
    if (position)
    {
        position->handle_trade_rtn(position, trade);
    }

    // how to get asset type
    zs_commission_model_t* comm_model;
    comm_model = zs_commission_model_get(blotter->Commission, 0);
    double comm = comm_model->calculate(comm_model, lorder, trade);

    blotter->Account->FundAccount.Commission += comm;

    return 0;
}


static void _zs_sync_price_to_positions(ztl_map_pair_t* pairs, int size, zs_bar_reader_t* barReader)
{
    for (int k = 0; k < size; ++k)
    {
        if (pairs[k].Value == NULL) {
            break;
        }

        double last_price = barReader->current(barReader, (zs_sid_t)pairs[k].Key, "close");
        if (last_price < 0.0001) {
            continue;
        }
        zs_position_sync_last_price(pairs[k].Value, last_price);
    }
}

int zs_handle_md_bar(zs_algorithm_t* algo, zs_bar_reader_t* bar_reader)
{
    // 遍历所有blotter的所有持仓，更新浮动盈亏等，最新价格等
    zs_blotter_t* blotter;

    for (uint32_t i = 0; i < ztl_array_size(&algo->BlotterMgr.BlotterArray); ++i)
    {
        blotter = (zs_blotter_t*)ztl_array_at((&algo->BlotterMgr.BlotterArray), i);

        ztl_map_pair_t pairs[1024] = { 0 };
        ztl_map_to_array(blotter->Positions, pairs, 1024);
        _zs_sync_price_to_positions(pairs, 1024, bar_reader);
    }

    return 0;
}

int zs_handle_md_tick(zs_algorithm_t* algo, zs_tick_t* tick)
{
    zs_sid_t sid;
    zs_blotter_t* blotter;
    zs_position_engine_t* position;
    sid = zs_asset_lookup(algo->AssetFinder, tick->ExchangeID, tick->Symbol, (int)strlen(tick->Symbol));

    // 遍历所有blotter，根据tickData找到更新浮动盈亏等，最新价格等
    for (uint32_t i = 0; i < ztl_array_size(&algo->BlotterMgr.BlotterArray); ++i)
    {
        blotter = (zs_blotter_t*)ztl_array_at((&algo->BlotterMgr.BlotterArray), i);
        position = zs_get_position_engine(blotter, sid);
        if (position)
        {
            zs_position_sync_last_price(position, tick->LastPrice);
        }
    }

    return 0;
}


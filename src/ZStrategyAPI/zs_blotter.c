#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_palloc.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_blotter.h"

#include "zs_data_portal.h"

#include "zs_position.h"

#include "zs_risk_control.h"


static void* _zs_order_sysid_keydup(void* priv, const void* key) {
    zs_blotter_t* blotter = (zs_blotter_t*)priv;
    return zs_str_keydup(key, ztl_palloc, blotter->Pool);
}

static dictType sysidHashDictType = {
    zs_str_hash,
    _zs_order_sysid_keydup,
    NULL,
    zs_str_cmp,
    NULL,
    NULL
};



zs_blotter_t* zs_blotter_create(zs_algorithm_t* algo)
{
    ztl_pool_t* pool;
    zs_blotter_t* blotter;

    pool = algo->Pool;
    blotter = (zs_blotter_t*)ztl_pcalloc(pool, sizeof(zs_blotter_t));
    blotter->Pool = pool;

    blotter->Algorithm = algo;

    blotter->OrderDict = dictCreate(&sysidHashDictType, blotter);
    blotter->WorkOrderList = zs_orderlist_create();

    ztl_array_init(&blotter->Trades, pool, 8192, sizeof(void*));

    blotter->Positions = ztl_map_create(64);

    blotter->pFundAccount = (zs_fund_account_t*)ztl_pcalloc(pool, sizeof(zs_fund_account_t));
    blotter->pPortfolio = (zs_portfolio_t*)ztl_pcalloc(pool, sizeof(zs_portfolio_t));

    blotter->Commission = zs_commission_create(blotter->Algorithm);

    // demo debug
    zs_fund_account_t* account = blotter->pFundAccount;
    strcpy(account->AccountID, "000100000002");
    account->Available = 1000000;
    account->Balance = 1000000;

    blotter->RiskControl = algo->RiskControl;

    //blotter->TradeApi = zs_broker_get_tradeapi()

    return blotter;
}

void zs_blotter_release(zs_blotter_t* blotter)
{
    if (!blotter) {
        return;
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
    rv = tdapi->order(tdapi->ApiInstrance, order_req);
    if (rv != 0)
    {
        // send order failed
        zs_order_t order = { 0 };
        strcpy(order.AccountID, order_req->AccountID);
        strcpy(order.Symbol, order_req->Symbol);
        order.Sid = order_req->Sid;
        order.Status = ZS_OS_Rejected;
        // other fields...
        zs_handle_order_returned(blotter, &order);
        return rv;
    }

    // todo: save this order 

    return rv;
}

int zs_blotter_quote_order(zs_blotter_t* blotter, zs_quote_order_req_t* quoteOrderReq)
{
    return -1;
}


zs_order_t* zs_get_order_byid(zs_blotter_t* blotter, int64_t orderId)
{
    zs_order_t* order;

    order = zs_order_find_byid(blotter->WorkOrderList, orderId);

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
    zs_fund_account_t*      account;
    zs_position_engine_t*   position;
    zs_contract_t*          contract;

    account = blotter->pFundAccount;

    // 自成交检测
    // blotter->WorkWorkOrderList

    // 资金处理

    // 持仓
    position = zs_get_position_engine(blotter, order_req->Sid);
    if (position)
    {
        zs_position_req_order(position, order_req);
    }

    // 资金

    /* 需要一个访问实时行情的接口 */
    return 0;
}

int zs_handle_quote_order_submit(zs_blotter_t* blotter,
    zs_quote_order_req_t* quoteOrderReq)
{
    return -1;
}

int zs_handle_order_returned(zs_blotter_t* blotter, zs_order_t* order)
{
    // 更新订单状态
    zs_order_t* lorder;
    lorder = zs_get_order_byid(blotter, order->OrderID);
    if (!lorder)
    {
        return -1;
    }

    ZSOrderStatus status = order->Status;
    if (status == ZS_OS_Filled || status == ZS_OS_Canceld || status == ZS_OS_Rejected)
    {
        ZStrKey key = { (int)strlen(order->OrderSysID), order->OrderSysID };
        if (dictFind(blotter->OrderDict, &key)) {
            return 0;
        }

        dictAdd(blotter->OrderDict, &key, 0);
    }

    strcpy(lorder->OrderSysID, order->OrderSysID);
    lorder->Filled = order->Filled;
    lorder->Status = order->Status;
    lorder->OrderTime = order->OrderTime;
    lorder->CancelTime = order->CancelTime;

    // zs_account_on_order_rtn(blotter->Account,)

    if (order->Status == ZS_OS_Canceld || order->Status == ZS_OS_PartCancled)
    {
        zs_position_engine_t* position;
        position = zs_get_position_engine(blotter, order->Sid);
        if (position)
        {
            zs_position_rtn_order(position, order);
        }
    }

    return 0;
}

int zs_handle_order_trade(zs_blotter_t* blotter, zs_trade_t* trade)
{
    // 开仓单成交：调整持仓，重新计算占用资金，计算持仓成本
    // 平仓单成交：解冻持仓，回笼资金
    zs_order_t* lorder;
    lorder = zs_get_order_byid(blotter, trade->OrderID);
    if (!lorder)
    {
        return -1;
    }

    lorder->Filled += trade->Volume;
    if (lorder->Filled == lorder->Quantity)
        lorder->Status = ZS_OS_Filled;
    else
        lorder->Status = ZS_OS_PartFilled;

    zs_position_engine_t*  position;
    position = zs_get_position_engine(blotter, trade->Sid);
    if (position)
    {
        zs_position_rtn_trade(position, trade);
    }

    // how to get asset type
    zs_commission_model_t* comm_model;
    comm_model = zs_commission_model_get(blotter->Commission, 0);
    float comm = comm_model->calculate(comm_model, lorder, trade);

    blotter->pFundAccount->Commission += comm;

    return 0;
}


static void _zs_sync_price_to_positions(ztl_map_pair_t* pairs, int size, zs_bar_reader_t* barReader)
{
    for (int k = 0; k < size; ++k)
    {
        if (pairs[k].Value == NULL) {
            break;
        }

        float last_price = barReader->current(barReader, (zs_sid_t)pairs[k].Key, "close");
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
    sid = zs_asset_lookup(algo->AssetFinder, tick->Symbol, (int)strlen(tick->Symbol));

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


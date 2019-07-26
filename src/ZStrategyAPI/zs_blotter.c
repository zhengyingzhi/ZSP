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


// convert order req as order object
void zs_convert_order_req(zs_order_t* order, const zs_order_req_t* order_req);


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

    blotter->account_conf = account_conf;
    blotter->IsSelfCalc = 1;        // currently default is 1
    blotter->Algorithm = algo;

    blotter->OrderDict = zs_orderdict_create(blotter->Pool);
    blotter->WorkOrderList = zs_orderlist_create();

    blotter->TradeDict = dictCreate(&strHashDictType, blotter->Pool);
    blotter->TradeArray = (ztl_array_t*)ztl_pcalloc(blotter->Pool, sizeof(ztl_array_t));
    ztl_array_init(blotter->TradeArray, NULL, 1024, sizeof(zs_trade_t*));

    blotter->Positions = dictCreate(&uintHashDictType, blotter);
    blotter->PositionArray = (ztl_array_t*)ztl_pcalloc(pool, sizeof(ztl_array_t));
    ztl_array_init(blotter->PositionArray, NULL, 64, sizeof(zs_position_engine_t*));

    blotter->Account = zs_account_create(blotter->Pool);
    blotter->pPortfolio = (zs_portfolio_t*)ztl_pcalloc(pool, sizeof(zs_portfolio_t));

    blotter->Commission = zs_commission_create(blotter->Algorithm);

#if 1
    // demo debug
    zs_fund_account_t* fund_account;
    fund_account = &blotter->Account->FundAccount;
    strcpy(fund_account->AccountID, account_conf->AccountID);
    fund_account->Available = 1000000;
    fund_account->Balance = 1000000;
#endif

    blotter->RiskControl = algo->RiskControl;

    // FIXME: the api object
    blotter->TradeApi = zs_broker_get_tradeapi(algo->Broker, account_conf->TradeApiName);
    if (blotter->TradeApi->create)
        blotter->TradeApi->ApiInstance = blotter->TradeApi->create("", 0);
    blotter->MdApi = zs_broker_get_mdapi(algo->Broker, account_conf->MDApiName);
    if (blotter->MdApi->create)
        blotter->MdApi->ApiInstance = blotter->MdApi->create("", 0);

    blotter->subscribe = zs_blotter_subscribe;
    blotter->order = zs_blotter_order;
    blotter->quote_order = zs_blotter_quote_order;
    blotter->cancel = zs_blotter_cancel;

    blotter->handle_order_submit = zs_blotter_handle_order_submit;
    blotter->handle_order_returned = zs_blotter_handle_order_returned;
    blotter->handle_order_trade = zs_blotter_handle_order_trade;
    blotter->handle_tick = zs_blotter_handle_tick;
    blotter->handle_bar = zs_blotter_handle_bar;

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
        zs_orderdict_release(blotter->OrderDict);
        blotter->OrderDict = NULL;
    }

    if (blotter->WorkOrderList) {
        zs_orderlist_create(blotter->WorkOrderList);
        blotter->WorkOrderList = NULL;
    }

    if (blotter->TradeDict) {
        dictRelease(blotter->TradeDict);
        blotter->TradeDict = NULL;
    }

    if (blotter->TradeArray) {
        ztl_array_release(blotter->TradeArray);
        blotter->TradeArray = NULL;
    }

    if (blotter->Positions) {
        dictRelease(blotter->Positions);
        blotter->Positions = NULL;
    }

    if (blotter->Commission) {
        zs_commission_release(blotter->Commission);
        blotter->Commission = NULL;
    }

}

void zs_blotter_stop(zs_blotter_t* blotter)
{
    // 处理停止时的一些信息
}

int zs_blotter_order(zs_blotter_t* blotter, zs_order_req_t* order_req)
{
    int rv;
    zs_trade_api_t* tdapi;

    if (!order_req->AccountID[0]) {
        strcpy(order_req->AccountID, blotter->Account->AccountID);
    }

    rv = zs_risk_control_check(blotter->Algorithm->RiskControl, order_req);
    if (rv != 0)
    {
        return rv;
    }

    rv = blotter->handle_order_submit(blotter, order_req);
    if (rv != 0) {
        return rv;
    }

    tdapi = blotter->TradeApi;
    rv = tdapi->order(tdapi->ApiInstance, order_req);
    if (rv != 0)
    {
        // send order failed
        zs_order_t order = { 0 };
        zs_convert_order_req(&order, order_req);

        blotter->handle_order_returned(blotter, &order);
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

    if (!cancel_req->AccountID[0]) {
        strcpy(cancel_req->AccountID, blotter->Account->AccountID);
    }

    tdapi = blotter->TradeApi;
    rv = tdapi->cancel(tdapi->ApiInstance, cancel_req);
    if (rv != 0)
    {
        // 
    }
    return rv;
}

int zs_blotter_subscribe(zs_blotter_t* blotter, zs_subscribe_t* sub_req)
{
    int rv;
    zs_md_api_t* mdapi;

    mdapi = blotter->MdApi;
    rv = mdapi->subscribe(mdapi->ApiInstance, &sub_req, 1);
    if (rv != 0)
    {
        //
    }

    return rv;
}

int zs_blotter_save_order(zs_blotter_t* blotter, zs_order_req_t* order_req)
{
    // 保存订单到 OrderDict 和 WorkOrderDict

    zs_order_t* order;
    order = (zs_order_t*)ztl_pcalloc(blotter->Pool, sizeof(zs_order_t));

    zs_convert_order_req(order, order_req);

    zs_orderdict_add_order(blotter->OrderDict, order);
    zs_orderlist_append(blotter->WorkOrderList, order);

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

zs_position_engine_t* zs_position_engine_get(zs_blotter_t* blotter, zs_sid_t sid)
{
    dictEntry* entry;
    zs_position_engine_t* pos_engine;
    entry = dictFind(blotter->Positions, (void*)sid);
    if (entry) {
        pos_engine = (zs_position_engine_t*)entry->v.val;
    }
    else {
        pos_engine = NULL;
    }
    return pos_engine;
}

zs_position_engine_t* zs_position_engine_get_ex(zs_blotter_t* blotter, zs_sid_t sid)
{
    zs_position_engine_t* pos_engine;
    pos_engine = zs_position_engine_get(blotter, sid);
    if (!pos_engine)
    {
        zs_contract_t* contract;
        contract = (zs_contract_t*)zs_asset_find_by_sid(blotter->Algorithm->AssetFinder, sid);
        pos_engine = zs_position_create(blotter, blotter->Pool, contract);

        if (pos_engine) {
            dictAdd(blotter->Positions, (void*)sid, pos_engine);
            ztl_array_push_back(blotter->PositionArray, &pos_engine);
        }
    }
    return pos_engine;
}



//////////////////////////////////////////////////////////////////////////
int zs_blotter_handle_account(zs_blotter_t* blotter, zs_fund_account_t* fund_account)
{
    // 资金查询应答
    blotter->TradingDay = fund_account->TradingDay;
    memcpy(&blotter->Account->FundAccount, fund_account, sizeof(zs_fund_account_t));
    return 0;
}

int zs_blotter_handle_position(zs_blotter_t* blotter, zs_position_t* pos)
{
    // 持仓查询应答，同一合约的持仓可能分今昨两条记录？
    zs_sid_t sid;
    zs_position_engine_t* pos_engine;

    if (!pos->Symbol[0]) {
        // 没有持仓
        return 0;
    }

    sid = zs_asset_lookup(blotter->Algorithm->AssetFinder, pos->ExchangeID,
        pos->Symbol, (int)strlen(pos->Symbol));
    if (sid == ZS_INVALID_SID) {
        // ERRORID: not find the asset
        return -1;
    }

    pos_engine = zs_position_engine_get_ex(blotter, sid);
    if (pos_engine)
    {
        pos_engine->handle_position_rsp(pos_engine, pos);
#if 0
        oldpos->Position += pos->Position;
        if (pos->PositionDate == ZS_PD_Yesterday)
            oldpos->YdPosition = pos->YdPosition;
        oldpos->PositionCost += pos->PositionCost;
        oldpos->OpenCost += pos->OpenCost;
        oldpos->PositionPnl += pos->PositionPnl;
        oldpos->UseMargin += pos->UseMargin;
        oldpos->Frozen += pos->Frozen;
        oldpos->FrozenMargin += pos->FrozenMargin;
        oldpos->FrozenCommission += pos->FrozenCommission;
#endif
    }
    else
    {
        // ERRORID:
    }

    return 0;
}

int zs_blotter_handle_position_detail(zs_blotter_t* blotter, zs_position_detail_t* pos_detail)
{
    zs_sid_t sid;
    zs_position_engine_t* pos_engine;

    if (!pos_detail->Symbol[0]) {
        // 没有持仓
        return 0;
    }

    sid = zs_asset_lookup(blotter->Algorithm->AssetFinder, pos_detail->ExchangeID,
        pos_detail->Symbol, (int)strlen(pos_detail->Symbol));
    if (sid == ZS_INVALID_SID) {
        // ERRORID: not find the asset
        return -1;
    }

    pos_engine = zs_position_engine_get(blotter, sid);
    if (!pos_engine) {
        // ERRORID: not position engine in processing pos detail
        return -1;
    }
    pos_engine->handle_pos_detail_rsp(pos_engine, pos_detail);

    return 0;
}

int zs_blotter_handle_qry_order(zs_blotter_t* blotter, zs_order_t* order)
{
    return 0;
}

int zs_blotter_handle_qry_trade(zs_blotter_t* blotter, zs_trade_t* trade)
{
    return 0;
}

int zs_blotter_handle_timer(zs_blotter_t* blotter, int64_t flag)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////
// 订单回报事件
int zs_blotter_handle_order_submit(zs_blotter_t* blotter, zs_order_req_t* order_req)
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
        position = zs_position_engine_get(blotter, order_req->Sid);
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

int zs_blotter_handle_quote_order_submit(zs_blotter_t* blotter,
    zs_quote_order_req_t* quote_req)
{
    return -1;
}

int zs_blotter_handle_order_returned(zs_blotter_t* blotter, zs_order_t* order)
{
    zs_order_t*     work_order;
    zs_order_t*     old_order;
    zs_contract_t*  contract;
    ZSOrderStatus   order_status;

    fprintf(stderr, "blotter handle_order symbol:%s, vol:%d, price:%.2lf, dir:%d, offset:%d, oid:%s\n",
        order->Symbol, order->OrderQty, order->OrderPrice, order->Direction, order->OffsetFlag, order->OrderID);

    contract = zs_asset_find_by_sid(blotter->Algorithm->AssetFinder, order->Sid);

    // 查找本地委托并更新，若为挂单，则更新到workorders中，否则从workorders中删除

    order_status = order->OrderStatus;

    old_order = zs_orderdict_find(blotter->OrderDict, order->FrontID, order->SessionID, order->OrderID);
    if (old_order)
    {
        if (is_finished_status(order_status))
            // if (status == ZS_OS_Filled || status == ZS_OS_Canceld || status == ZS_OS_Rejected)
        {
            // 重复订单
            if (order_status == old_order->OrderStatus) {
                return 0;
            }
        }

        // 更新订单状态等数据
    }
    else
    {
        // we should make a copy order object
        zs_order_t* dup_order;
        dup_order = (zs_order_t*)ztl_palloc(blotter->Pool, sizeof(zs_order_t));
        ztl_memcpy(dup_order, order, sizeof(zs_order_t));
        zs_orderdict_add_order(blotter->OrderDict, dup_order);
    }

    // working order
    work_order = zs_get_order_by_sysid(blotter, order->ExchangeID, order->OrderSysID);
    if (!work_order)
    {
        return -1;
    }

    strcpy(work_order->OrderSysID, order->OrderSysID);
    work_order->FilledQty = order->FilledQty;
    work_order->OrderStatus = order->OrderStatus;
    work_order->OrderTime = order->OrderTime;
    work_order->CancelTime = order->CancelTime;

    // 是否自动维护模式
    if (!blotter->IsSelfCalc) {
        return 0;
    }

    zs_account_on_order_rtn(blotter->Account, work_order, contract);

    if (is_finished_status(order->OrderStatus))
    {
        zs_position_engine_t* position;
        position = zs_position_engine_get(blotter, order->Sid);
        if (position)
        {
            position->handle_order_rtn(position, order);
        }
    }

    return 0;
}

int zs_blotter_handle_order_trade(zs_blotter_t* blotter, zs_trade_t* trade)
{
    // 开仓单成交：调整持仓，重新计算占用资金，计算持仓成本
    // 平仓单成交：解冻持仓，回笼资金
    zs_contract_t*          contract;
    zs_order_t*             work_order;
    zs_position_engine_t*   pos_engine;
    char zs_tradeid[32];    // 过滤重复成交

    fprintf(stderr, "blotter handle_trade symbol:%s, tid:%s, qty:%d, px:%.2lf\n",
        trade->Symbol, trade->TradeID, trade->Volume, trade->Price);

    int len = zs_make_id(zs_tradeid, trade->ExchangeID, trade->TradeID);
    dictEntry* entry = zs_strdict_find(blotter->TradeDict, zs_tradeid, len);
    if (entry) {
        // 重复的成交回报
        return 1;
    }

    ZStrKey* key = zs_str_keydup2(zs_tradeid, len, ztl_palloc, blotter->Pool);
    zs_trade_t* dup_trade = (zs_trade_t*)ztl_palloc(blotter->Pool, sizeof(zs_trade_t));
    ztl_memcpy(dup_trade, trade, sizeof(zs_trade_t));
    dictAdd(blotter->TradeDict, key, trade);

    ztl_array_push_back(blotter->TradeArray, &trade);

    // 原始挂单
    work_order = zs_get_order_by_sysid(blotter, trade->ExchangeID, trade->OrderSysID);
    if (!work_order)
    {
        // ERRORID: not find original order
        return -1;
    }

    contract = zs_asset_find_by_sid(blotter->Algorithm->AssetFinder, trade->Sid);

    work_order->FilledQty += trade->Volume;
    if (work_order->FilledQty == work_order->OrderQty)
        work_order->OrderStatus = ZS_OS_Filled;
    else
        work_order->OrderStatus = ZS_OS_PartFilled;

    // 是否自动维护模式
    if (blotter->IsSelfCalc)
    {
        pos_engine = zs_position_engine_get_ex(blotter, trade->Sid);
        if (!pos_engine)
        {
            // ERRORID: not find this asset
            return -1;
        }
        pos_engine->handle_trade_rtn(pos_engine, trade);
    }

    // get commission model by asset type
    zs_commission_model_t* comm_model;
    comm_model = zs_commission_model_get(blotter->Commission, contract->ProductClass != ZS_PC_Future);
    double comm = comm_model->calculate(comm_model, work_order, trade);

    trade->Commission = comm;
    trade->FrontID = work_order->FrontID;
    trade->SessionID = work_order->SessionID;

    blotter->Account->FundAccount.Commission += comm;

    return 0;
}


// 行情事件
static void _zs_sync_price_to_positions(ztl_map_pair_t* pairs, int size, zs_bar_reader_t* bar_reader)
{
    double last_price;
    for (int k = 0; k < size; ++k)
    {
        if (!pairs[k].Value) {
            break;
        }

        last_price = bar_reader->current2(bar_reader, (zs_sid_t)pairs[k].Key, ZS_FT_Close);
        if (last_price < 0.0001) {
            continue;
        }
        zs_position_sync_last_price(pairs[k].Value, last_price);
    }
}

int zs_blotter_handle_tick(zs_blotter_t* blotter, zs_tick_t* tick)
{
    zs_sid_t sid;
    zs_position_engine_t* position;
    sid = tick->Sid;

    position = zs_position_engine_get(blotter, sid);
    if (position)
    {
        zs_position_sync_last_price(position, tick->LastPrice);
    }

    return 0;
}

int zs_blotter_handle_bar(zs_blotter_t* blotter, zs_bar_reader_t* bar_reader)
{
    double last_price;

    // 遍历所有blotter的所有持仓，更新浮动盈亏等，最新价格等
    if (bar_reader->DataPortal)
    {
        zs_position_engine_t* pos_engine;
        for (uint32_t i = 0; i < ztl_array_size(blotter->PositionArray); ++i)
        {
            pos_engine = (zs_position_engine_t*)ztl_array_at2(blotter->PositionArray, i);
            if (!pos_engine)
                continue;

            last_price = bar_reader->current2(bar_reader, pos_engine->Sid, ZS_FT_Close);
            if (last_price < 0.0001) {
                continue;
            }
            pos_engine->sync_last_price(pos_engine, last_price);
        }
    }
    else
    {
        zs_sid_t sid;
        zs_position_engine_t* position;

        sid = bar_reader->Bar.Sid;
        position = zs_position_engine_get(blotter, bar_reader->Bar.Sid);
        if (position)
        {
            last_price = bar_reader->current2(bar_reader, sid, ZS_FT_Close);
            zs_position_sync_last_price(position, last_price);
        }
    }

    return 0;
}

void zs_convert_order_req(zs_order_t* order, const zs_order_req_t* order_req)
{
    strcpy(order->AccountID, order_req->AccountID);
    strcpy(order->BrokerID, order_req->BrokerID);
    strcpy(order->Symbol, order_req->Symbol);
    strcpy(order->UserID, order_req->UserID);
    strcpy(order->OrderID, order_req->OrderID);
    order->ExchangeID   = order_req->ExchangeID;
    order->Sid          = order_req->Sid;
    order->OrderQty     = order_req->OrderQty;
    order->OrderPrice   = order_req->OrderPrice;
    order->Direction    = order_req->Direction;
    order->OffsetFlag   = order_req->OffsetFlag;
    order->OrderType    = order_req->OrderType;
    // order->TradingDay = blotter->TradingDay;
    order->OrderStatus  = ZS_OS_Rejected;

    order->FrontID      = order_req->FrontID;
    order->SessionID    = order_req->SessionID;
}

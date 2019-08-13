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
        zs_log_error(algo->Log, "blotter: create not find account conf for account:%s", accountid);
        return NULL;
    }

    if (!account_conf->TradeAddr[0])
    {
        zs_conf_broker_t* broker_conf;
        broker_conf = zs_configs_find_broker(algo->Params, account_conf->BrokerID);
        if (!broker_conf) {
            zs_log_error(algo->Log, "blotter: create not find broker conf for account:%s,broker:%s",
                accountid, account_conf->BrokerID);
            return NULL;
        }
        else {
            strcpy(account_conf->TradeAddr, broker_conf->TradeAddr);
            strcpy(account_conf->MDAddr, broker_conf->MDAddr);
        }
    }

    pool = ztl_create_pool(ZTL_DEFAULT_POOL_SIZE);
    blotter = (zs_blotter_t*)ztl_pcalloc(algo->Pool, sizeof(zs_blotter_t));
    blotter->Pool = pool;

    blotter->AccountConf = account_conf;
    blotter->IsSelfCalc = 1;        // currently default is 1
    blotter->Algorithm = algo;
    blotter->Log = algo->Log;

    blotter->OrderDict = zs_orderdict_create(blotter->Pool);
    blotter->WorkOrderList = zs_orderlist_create();

    blotter->TradeDict = dictCreate(&strHashDictType, blotter->Pool);
    blotter->TradeArray = (ztl_array_t*)ztl_pcalloc(algo->Pool, sizeof(ztl_array_t));
    ztl_array_init(blotter->TradeArray, NULL, 1024, sizeof(zs_trade_t*));

    blotter->PositionDict = dictCreate(&uintHashDictType, blotter);
    blotter->PositionArray = (ztl_array_t*)ztl_pcalloc(algo->Pool, sizeof(ztl_array_t));
    ztl_array_init(blotter->PositionArray, NULL, 64, sizeof(zs_position_engine_t*));

    blotter->Account = zs_account_create(blotter);
    blotter->pPortfolio = (zs_portfolio_t*)ztl_pcalloc(algo->Pool, sizeof(zs_portfolio_t));

    blotter->Commission = zs_commission_create(blotter->Algorithm);

    blotter->RiskControl = algo->RiskControl;

    // FIXME: the api object
    blotter->TradeApi = zs_broker_get_tradeapi(algo->Broker, account_conf->TradeAPIName);
    if (!blotter->TradeApi) {
        zs_log_error(algo->Log, "blotter: create no tradeapi for account:%s, apiname:%s\n",
            accountid, account_conf->TradeAPIName);
        return NULL;
    }

    if (blotter->TradeApi->create) {
        blotter->TradeApi->ApiInstance = blotter->TradeApi->create("", 0);
    }

    blotter->MdApi = zs_broker_get_mdapi(algo->Broker, account_conf->MDAPIName);
    if (blotter->MdApi && blotter->MdApi->create) {
        blotter->MdApi->ApiInstance = blotter->MdApi->create("", 0);
    }

    blotter->subscribe      = zs_blotter_subscribe;
    blotter->subscribe_batch= zs_blotter_subscribe_batch;
    blotter->order          = zs_blotter_order;
    blotter->quote_order    = zs_blotter_quote_order;
    blotter->cancel         = zs_blotter_cancel;

    blotter->handle_order_req   = zs_blotter_handle_order_req;
    blotter->handle_order_rtn   = zs_blotter_handle_order_rtn;
    blotter->handle_trade_rtn   = zs_blotter_handle_trade_rtn;

    blotter->handle_tick    = zs_blotter_handle_tick;
    blotter->handle_tickl2  = NULL;
    blotter->handle_bar     = zs_blotter_handle_bar;
    blotter->handle_timer   = zs_blotter_handle_timer;

    return blotter;
}

void zs_blotter_release(zs_blotter_t* blotter)
{
    if (!blotter) {
        return;
    }

    zs_log_info(blotter->Log, "blotter: release for %s", blotter->Account->AccountID);

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

    if (blotter->PositionDict) {
        dictRelease(blotter->PositionDict);
        blotter->PositionDict = NULL;
    }

    if (blotter->PositionArray) {
        ztl_array_release(blotter->PositionArray);
        blotter->PositionArray = NULL;
    }

    if (blotter->Commission) {
        zs_commission_release(blotter->Commission);
        blotter->Commission = NULL;
    }

}

void zs_blotter_stop(zs_blotter_t* blotter)
{
    // 处理停止时的一些信息
    zs_log_info(blotter->Log, "blotter: stop for %s", blotter->Account->AccountID);

}


int zs_blotter_trade_connect(zs_blotter_t* blotter)
{
    zs_trade_api_t* tdapi;

    zs_log_info(blotter->Log, "blotter: trade_connect for account:%s", blotter->Account->AccountID);

    tdapi = blotter->TradeApi;
    if (!tdapi) {
        // no trader api
        return -1;
    }

    tdapi->UserData = blotter->Algorithm;

    if (!tdapi->ApiInstance)
        tdapi->ApiInstance = tdapi->create("", 0);
    tdapi->regist(tdapi->ApiInstance, &td_handlers, tdapi, blotter->AccountConf);
    tdapi->connect(blotter->TradeApi->ApiInstance, NULL);

    return 0;
}

int zs_blotter_md_connect(zs_blotter_t* blotter)
{
    zs_md_api_t* mdapi;

    zs_log_info(blotter->Log, "blotter: md_connect for account:%s", blotter->Account->AccountID);

    mdapi = blotter->MdApi;
    if (!mdapi) {
        // no trader api
        return -1;
    }

    mdapi->UserData = blotter->Algorithm;

    if (!mdapi->ApiInstance)
        mdapi->ApiInstance = mdapi->create("", 0);
    mdapi->regist(mdapi->ApiInstance, &md_handlers, mdapi, blotter->AccountConf);
    mdapi->connect(mdapi->ApiInstance, NULL);

    return 0;
}

int zs_blotter_order(zs_blotter_t* blotter, zs_order_req_t* order_req)
{
    int rv;
    zs_trade_api_t* tdapi;

    if (!order_req->AccountID[0]) {
        strcpy(order_req->AccountID, blotter->Account->AccountID);
    }

    rv = zs_risk_control_check(blotter->Algorithm->RiskControl, order_req);
    if (rv != ZS_OK) {
        goto ORDER_END;
    }

    rv = blotter->handle_order_req(blotter, order_req);
    if (rv != ZS_OK) {
        goto ORDER_END;
    }

    tdapi = blotter->TradeApi;
    rv = tdapi->order(tdapi->ApiInstance, order_req);
    if (rv != ZS_OK)
    {
        // send order failed
        zs_order_t order = { 0 };
        zs_convert_order_req(&order, order_req);
        order.OrderStatus = ZS_OS_Rejected;

        blotter->handle_order_rtn(blotter, &order);
        goto ORDER_END;
    }

    // save this order 
    zs_blotter_save_order(blotter, order_req);

ORDER_END:
    zs_log_info(blotter->Log, "blotter: order rv:%d", rv);
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
        zs_log_error(blotter->Log, "blotter: subscribe failed:%d", rv);
    }

    return rv;
}

int zs_blotter_subscribe_batch(zs_blotter_t* blotter, zs_subscribe_t* sub_reqs[], int count)
{
    int rv;
    zs_md_api_t* mdapi;

    mdapi = blotter->MdApi;
    rv = mdapi->subscribe(mdapi->ApiInstance, sub_reqs, count);
    if (rv != 0)
    {
        zs_log_error(blotter->Log, "blotter: subscribe_batch failed:%d", rv);
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

    return ZS_OK;
}

zs_order_t* zs_get_order_by_sysid(zs_blotter_t* blotter, ZSExchangeID exchange_id, const char* order_sysid)
{
    zs_order_t* order;
    order = zs_order_find_by_sysid(blotter->WorkOrderList, exchange_id, order_sysid);
    return order;
}

zs_order_t* zs_get_order_by_id(zs_blotter_t* blotter, int32_t frontid, int32_t sessionid, const char* orderid)
{
    zs_order_t* order;
    order = zs_order_find(blotter->WorkOrderList, frontid, sessionid, orderid);
    return order;
}

zs_position_engine_t* zs_position_engine_get(zs_blotter_t* blotter, zs_sid_t sid)
{
    dictEntry* entry;
    zs_position_engine_t* pos_engine;
    entry = dictFind(blotter->PositionDict, (void*)sid);
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
        pos_engine = zs_position_create(blotter, blotter->Algorithm->Pool, contract);

        if (pos_engine) {
            dictAdd(blotter->PositionDict, (void*)sid, pos_engine);
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
    zs_account_fund_update(blotter->Account, fund_account);

    zs_log_info(blotter->Log, "blotter: fund account bal:%.2lf, avail:%.2lf, frozen:%.2lf, margin:%.2lf\n",
        fund_account->Balance, fund_account->Available, fund_account->FrozenCash, fund_account->Margin);
    return ZS_OK;
}

int zs_blotter_handle_position(zs_blotter_t* blotter, zs_position_t* pos)
{
    // 持仓查询应答，同一合约的持仓可能分今昨两条记录？
    zs_sid_t sid;
    zs_position_engine_t* pos_engine;

    if (!pos->Symbol[0]) {
        // 没有持仓
        return ZS_OK;
    }

    zs_log_info(blotter->Log, "blotter: position symbol:%s, pos:%d, price:%.2lf, margin:%.2lf\n",
        pos->Symbol, pos->Position, pos->PositionPrice, pos->UseMargin);

    sid = zs_asset_lookup(blotter->Algorithm->AssetFinder, pos->ExchangeID,
        pos->Symbol, (int)strlen(pos->Symbol));
    if (sid == ZS_SID_INVALID) {
        // ERRORID: not find the asset
        return ZS_ERR_NoAsset;
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

    return ZS_OK;
}

int zs_blotter_handle_position_detail(zs_blotter_t* blotter, zs_position_detail_t* pos_detail)
{
    zs_sid_t sid;
    zs_position_engine_t* pos_engine;

    if (!pos_detail->Symbol[0]) {
        // 没有持仓
        return ZS_OK;
    }

    zs_log_info(blotter->Log, "blotter: position detail symbol:%s, vol:%d, open_price:%.2lf, pos_price:%.2lf, open_date:%d\n",
         pos_detail->Symbol, pos_detail->Volume, pos_detail->OpenPrice, pos_detail->PositionPrice, pos_detail->OpenDate);

    sid = zs_asset_lookup(blotter->Algorithm->AssetFinder, pos_detail->ExchangeID,
        pos_detail->Symbol, (int)strlen(pos_detail->Symbol));
    if (sid == ZS_SID_INVALID) {
        return ZS_ERR_NoAsset;
    }

    pos_engine = zs_position_engine_get(blotter, sid);
    if (!pos_engine) {
        return ZS_ERR_NoPosEngine;
    }
    pos_engine->handle_pos_detail_rsp(pos_engine, pos_detail);

    return ZS_OK;
}

int zs_blotter_handle_qry_order(zs_blotter_t* blotter, zs_order_t* order)
{
    return ZS_OK;
}

int zs_blotter_handle_qry_trade(zs_blotter_t* blotter, zs_trade_t* trade)
{
    return ZS_OK;
}

int zs_blotter_handle_timer(zs_blotter_t* blotter, int64_t flag)
{
    return ZS_OK;
}

//////////////////////////////////////////////////////////////////////////
// 订单回报事件
int zs_blotter_handle_order_req(zs_blotter_t* blotter, zs_order_req_t* order_req)
{
    /*
     * 处理订单提交请求:
     * 1. 风控
     * 2. 开仓-冻结资金,手续费
     */
    int             rv;
    zs_account_t*   account;
    zs_contract_t*  contract;

    if (order_req->Contract) {
        contract = (zs_contract_t*)order_req->Contract;
    }
    else {
        contract = zs_asset_find_by_sid(blotter->Algorithm->AssetFinder, order_req->Sid);
        order_req->Contract = contract;
    }

    if (!contract)
    {
        zs_log_error(blotter->Log, "blotter: handle_order_req not find contract for account:%s, symbol:%s, sid:%ld\n",
            order_req->AccountID, order_req->Symbol, order_req->Sid);
        return ZS_ERR_NoContract;
    }

    zs_log_info(blotter->Log, "blotter: handle_order_req for account:%s, symbol:%s, qty:%d, px:%.2lf, dir:%d, offset:%d, sid:%ld\n",
        order_req->AccountID, order_req->Symbol, order_req->OrderQty, order_req->OrderPrice, order_req->Direction, order_req->OffsetFlag, order_req->Sid);

    account = blotter->Account;

    // 自成交检测
    // blotter->WorkWorkOrderList

    // 平仓请求，则待委托确认后再冻结

    // 资金
    rv = zs_account_handle_order_req(account, order_req, contract);
    if (rv != ZS_OK) {
        return rv;
    }

    return ZS_OK;
}

int zs_blotter_handle_quote_order_req(zs_blotter_t* blotter,
    zs_quote_order_req_t* quote_req)
{
    return ZS_ERR_NotImpl;
}

int zs_blotter_handle_order_rtn(zs_blotter_t* blotter, zs_order_t* order)
{
    zs_order_t*     work_order;
    zs_order_t*     old_order;
    zs_contract_t*  contract;
    ZSOrderStatus   order_status;
    zs_position_engine_t* pos_engine;

    zs_log_info(blotter->Log, "blotter: handle_order symbol:%s, qty:%d, price:%.2lf, dir:%d, offset:%d, oid:%s, status:%d\n",
        order->Symbol, order->OrderQty, order->OrderPrice, order->Direction, order->OffsetFlag, order->OrderID, order->OrderStatus);

    contract = zs_asset_find_by_sid(blotter->Algorithm->AssetFinder, order->Sid);
    pos_engine = NULL;

    // 查找本地委托并更新，若为挂单，则更新到workorders中，否则从workorders中删除

    order_status = order->OrderStatus;

    old_order = zs_get_order_by_id(blotter, order->FrontID, order->SessionID, order->OrderID);
    if (old_order)
    {
        if (is_finished_status(order_status))
        {
            // 重复订单
            if (order_status == old_order->OrderStatus) {
                return ZS_OK;
            }
        }

        // 订单确认回报
        if (order->OffsetFlag != ZS_OF_Open && old_order->OrderStatus == ZS_OS_Unknown)
        {
            pos_engine = zs_position_engine_get(blotter, order->Sid);
        }
    }
    else
    {
        // we should make a copy order object
        zs_order_t* dup_order;
        dup_order = (zs_order_t*)ztl_palloc(blotter->Pool, sizeof(zs_order_t));
        ztl_memcpy(dup_order, order, sizeof(zs_order_t));

        // the order maybe from other client
        zs_orderdict_add_order(blotter->OrderDict, dup_order);
        zs_orderlist_append(blotter->WorkOrderList, dup_order);

        if (order->OffsetFlag != ZS_OF_Open)
        {
            pos_engine = zs_position_engine_get(blotter, order->Sid);
        }
    }

    // working order
    work_order = zs_get_order_by_sysid(blotter, order->ExchangeID, order->OrderSysID);
    if (!work_order)
    {
        return ZS_ERR_NoOrder;
    }

    // 首次委托确认，且为平仓单
    if (pos_engine)
    {
        pos_engine->handle_order_req(pos_engine, order->Direction, order->OffsetFlag, order->OrderQty);
    }

    // 更新委托状态
    if (!work_order->OrderSysID[0])
    {
        strcpy(work_order->OrderSysID, order->OrderSysID);
        work_order->OrderDate = order->OrderDate;
        work_order->OrderTime = order->OrderTime;
    }
    work_order->FilledQty   = order->FilledQty;
    work_order->AvgPrice    = order->AvgPrice;
    work_order->OrderStatus = order->OrderStatus;
    work_order->CancelTime  = order->CancelTime;

    // 是否自动维护模式
    if (!blotter->IsSelfCalc) {
        return ZS_OK;
    }

    if (is_finished_status(order->OrderStatus))
    {
        // 资金处理
        zs_account_handle_order_finished(blotter->Account, work_order, contract);

        // 持仓处理
        pos_engine = zs_position_engine_get(blotter, order->Sid);
        if (pos_engine)
        {
            pos_engine->handle_order_rtn(pos_engine, order);
        }
    }

    return ZS_OK;
}

int zs_blotter_handle_trade_rtn(zs_blotter_t* blotter, zs_trade_t* trade)
{
    // 开仓单成交：调整持仓，重新计算占用资金，计算持仓成本
    // 平仓单成交：解冻持仓，回笼资金
    zs_contract_t*          contract;
    zs_order_t*             work_order;
    zs_position_engine_t*   pos_engine;
    char zs_tradeid[32];    // 过滤重复成交

    zs_log_info(blotter->Log, "blotter: handle_trade symbol:%s, tid:%s, qty:%d, px:%.2lf\n",
        trade->Symbol, trade->TradeID, trade->Volume, trade->Price);

    int len = zs_make_id(zs_tradeid, trade->ExchangeID, trade->TradeID);
    dictEntry* entry = zs_strdict_find(blotter->TradeDict, zs_tradeid, len);
    if (entry) {
        // 重复的成交回报
        return ZS_EXISTED;
    }

    ZStrKey* key = zs_str_keydup2(zs_tradeid, len, ztl_palloc, blotter->Pool);
    zs_trade_t* dup_trade = (zs_trade_t*)ztl_palloc(blotter->Pool, sizeof(zs_trade_t));
    ztl_memcpy(dup_trade, trade, sizeof(zs_trade_t));
    dictAdd(blotter->TradeDict, key, trade);

    ztl_array_push_back(blotter->TradeArray, &trade);

    // 原始挂单
    work_order = zs_get_order_by_sysid(blotter, trade->ExchangeID, trade->OrderSysID);
    if (!work_order) {
        return ZS_ERR_NoOrder;
    }

    contract = zs_asset_find_by_sid(blotter->Algorithm->AssetFinder, trade->Sid);

    work_order->FilledQty += trade->Volume;
    if (work_order->FilledQty == work_order->OrderQty)
        work_order->OrderStatus = ZS_OS_Filled;
    else
        work_order->OrderStatus = ZS_OS_PartFilled;

    trade->FrontID = work_order->FrontID;
    trade->SessionID = work_order->SessionID;

    // 是否自动维护模式
    if (blotter->IsSelfCalc)
    {
        pos_engine = zs_position_engine_get_ex(blotter, trade->Sid);
        if (!pos_engine)
        {
            zs_log_warn(blotter->Log, "blotter: handle_trade_rtn get position engine failed for account:%s, symbol:%s, sid:%ld",
                blotter->Account->AccountID, trade->Symbol, trade->Sid);
            return ZS_ERR_NoPosEngine;
        }
        pos_engine->handle_trade_rtn(pos_engine, trade);

        zs_log_info(blotter->Log, "blotter: handle_trade_rtn now  position for account:%s, symbol:%s, long:%d,short:%d",
            blotter->Account->AccountID, trade->Symbol, pos_engine->LongPos, pos_engine->ShortPos);
    }

    // FIXME: get commission
    double commission;
    commission = zs_commission_calculate(blotter->Commission, contract->ProductClass, work_order, trade);
    trade->Commission = commission;
    blotter->Account->FundAccount.Commission += commission;

    return ZS_OK;
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

    return ZS_OK;
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
    order->OrderStatus  = ZS_OS_Unknown;

    order->FrontID      = order_req->FrontID;
    order->SessionID    = order_req->SessionID;
}

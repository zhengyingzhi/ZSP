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


static void zs_convert_order_req(zs_order_t* order, const zs_order_req_t* order_req);


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

    blotter->OrderDict = zs_orderdict_create(blotter->Pool);
    blotter->WorkOrderList = zs_orderlist_create();

    blotter->TradeDict = dictCreate(&strHashDictType, blotter->Pool);
    blotter->TradeArray = (ztl_array_t*)ztl_pcalloc(blotter->Pool, sizeof(ztl_array_t));
    ztl_array_init(blotter->TradeArray, NULL, 1024, sizeof(zs_trade_t*));

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

    // FIXME: the api object
    blotter->TradeApi = zs_broker_get_tradeapi(algo->Broker, account_conf->TradeApiName);
    if (blotter->TradeApi->create)
        blotter->TradeApi->ApiInstance = blotter->TradeApi->create("", 0);
    blotter->MdApi = zs_broker_get_mdapi(algo->Broker, account_conf->MDApiName);
    if (blotter->MdApi->create)
        blotter->MdApi->ApiInstance = blotter->MdApi->create("", 0);

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
        ztl_map_release(blotter->Positions);
        blotter->Positions = NULL;
    }

    if (blotter->Commission) {
        zs_commission_release(blotter->Commission);
        blotter->Commission = NULL;
    }

}

void zs_blotter_stop(zs_blotter_t* blotter)
{
    // ����ֹͣʱ��һЩ��Ϣ
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
    // ��������
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

int zs_blotter_save_order(zs_blotter_t* blotter, zs_order_req_t* order_req)
{
    // ���涩���� OrderDict �� WorkOrderDict

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

zs_position_engine_t* zs_get_position_engine(zs_blotter_t* blotter, zs_sid_t sid)
{
    zs_position_engine_t* pos;
    pos = ztl_map_find(blotter->Positions, sid);
    return pos;
}


//////////////////////////////////////////////////////////////////////////
int zs_blotter_handle_account(zs_blotter_t* blotter, zs_fund_account_t* fund_account)
{
    memcpy(&blotter->Account->FundAccount, fund_account, sizeof(zs_fund_account_t));
    return 0;
}

int zs_blotter_handle_position(zs_blotter_t* blotter, zs_position_t* pos)
{
    // TODO: process position
    return 0;
}

int zs_blotter_handle_position_detail(zs_blotter_t* blotter, zs_position_detail_t* pos_detail)
{
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
// �����ر��¼�
int zs_blotter_handle_order_submit(zs_blotter_t* blotter, zs_order_req_t* order_req)
{
    // �������ύ����:
    // 1. ���
    // 2. ����-�����ʽ�, ƽ��-����ֲ�
    // 3. ������
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

    // �Գɽ����
    // blotter->WorkWorkOrderList

    // �ֲ�
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

    // �ʽ�
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

    contract = zs_asset_find_by_sid(blotter->Algorithm->AssetFinder, order->Sid);

    // ���ұ���ί�в����£���Ϊ�ҵ�������µ�workorders�У������workorders��ɾ��

    order_status = order->OrderStatus;

    old_order = zs_orderdict_find(blotter->OrderDict, order->FrontID, order->SessionID, order->OrderID);
    if (old_order)
    {
        if (is_finished_status(order_status))
            // if (status == ZS_OS_Filled || status == ZS_OS_Canceld || status == ZS_OS_Rejected)
        {
            // �ظ�����
            if (order_status == old_order->OrderStatus) {
                return 0;
            }
        }

        // ���¶���״̬������
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

    zs_account_on_order_rtn(blotter->Account, work_order, contract);

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

int zs_blotter_handle_order_trade(zs_blotter_t* blotter, zs_trade_t* trade)
{
    // ���ֵ��ɽ��������ֲ֣����¼���ռ���ʽ𣬼���ֲֳɱ�
    // ƽ�ֵ��ɽ����ⶳ�ֲ֣������ʽ�
    zs_contract_t* contract;
    zs_order_t* work_order;

    // �����ظ��ɽ�
    char zs_tradeid[32];

    int len = zs_make_id(zs_tradeid, trade->ExchangeID, trade->TradeID);
    dictEntry* entry = zs_strdict_find(blotter->TradeDict, zs_tradeid, len);
    if (entry) {
        // �ظ��ĳɽ��ر�
        return 1;
    }

    ZStrKey* key = zs_str_keydup2(zs_tradeid, len, ztl_palloc, blotter->Pool);
    zs_trade_t* dup_trade = (zs_trade_t*)ztl_palloc(blotter->Pool, sizeof(zs_trade_t));
    ztl_memcpy(dup_trade, trade, sizeof(zs_trade_t));
    dictAdd(blotter->TradeDict, key, trade);

    ztl_array_push_back(blotter->TradeArray, trade);

    // ԭʼ�ҵ�
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

    zs_position_engine_t*  position;
    position = zs_get_position_engine(blotter, trade->Sid);
    if (!position)
    {
        position = zs_position_create(blotter->Pool, contract);
    }
    position->handle_trade_rtn(position, trade);

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


// �����¼�
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

    position = zs_get_position_engine(blotter, sid);
    if (position)
    {
        zs_position_sync_last_price(position, tick->LastPrice);
    }

    return 0;
}

int zs_blotter_handle_bar(zs_blotter_t* blotter, zs_bar_reader_t* bar_reader)
{
    // ��������blotter�����гֲ֣����¸���ӯ���ȣ����¼۸��
    if (bar_reader->DataPortal)
    {
        ztl_map_pair_t pairs[1024] = { 0 };
        ztl_map_to_array(blotter->Positions, pairs, 1024);
        _zs_sync_price_to_positions(pairs, 1024, bar_reader);
    }
    else
    {
        double last_price;
        zs_sid_t sid;
        zs_position_engine_t* position;

        sid = bar_reader->Bar.Sid;
        position = zs_get_position_engine(blotter, bar_reader->Bar.Sid);
        if (position)
        {
            last_price = bar_reader->current2(bar_reader, sid, ZS_FT_Close);
            zs_position_sync_last_price(position, last_price);
        }
    }

    return 0;
}

static void zs_convert_order_req(zs_order_t* order, const zs_order_req_t* order_req)
{
    strcpy(order->AccountID, order_req->AccountID);
    strcpy(order->BrokerID, order_req->BrokerID);
    strcpy(order->Symbol, order_req->Symbol);
    strcpy(order->UserID, order_req->UserID);
    order->ExchangeID = order_req->ExchangeID;
    order->Sid = order_req->Sid;
    order->OrderQty = order_req->OrderQty;
    order->OrderPrice = order_req->OrderPrice;
    order->Direction = order_req->Direction;
    order->OffsetFlag = order_req->OffsetFlag;
    order->OrderType = order_req->OrderType;
    // order->TradingDay = blotter->TradingDay;
    order->OrderStatus = ZS_OS_Rejected;

    order->FrontID = order_req->FrontID;
    order->SessionID = order_req->SessionID;
}

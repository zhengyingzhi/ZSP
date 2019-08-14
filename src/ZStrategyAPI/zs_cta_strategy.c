#include <stdarg.h>

#include <ZToolLib/ztl_dyso.h>
#include <ZToolLib/ztl_times.h>
#include <ZToolLib/ztl_threads.h>

#include "zs_algorithm.h"
#include "zs_assets.h"
#include "zs_configs.h"

#include "zs_constants_helper.h"
#include "zs_data_portal.h"
#include "zs_event_engine.h"
#include "zs_cta_strategy.h"
#include "zs_strategy_engine.h"



zs_cta_strategy_t* zs_cta_strategy_create(zs_strategy_engine_t* zse, const char* setting, uint32_t strategy_id)
{
    zs_json_t* zjson;
    zs_cta_strategy_t* strategy;
    char strategy_name[16];
    char account_id[16];
    // char symbol[32];

    strategy = ztl_pcalloc(zse->Pool, sizeof(zs_cta_strategy_t));

    strategy->StrategyID = strategy_id;
    strategy->RunStatus = ZS_RS_Unknown;

    strategy->StrategySetting = setting;

    // ananlyze the setting
    zjson = zs_json_parse(setting, 0);
    if (!zjson) {
        // ERRORID: invalid strategy setting buffer
        return NULL;
    }

    memset(strategy_name, 0, sizeof(strategy_name));
    zs_json_get_string(zjson, "StrategyName", strategy_name, sizeof(strategy_name));
    if (!strategy_name[0]) {
        // ERRORID: no strategy_name in strategy setting
        return NULL;
    }

    memset(account_id, 0, sizeof(account_id));
    zs_json_get_string(zjson, "AccountID", account_id, sizeof(account_id));
    if (!account_id[0]) {
        // ERRORID: no account id in strategy setting
        zs_log_error(zse->Log, "cta: strategy_create no 'AccountID' in setting");
        return NULL;
    }

#if 0
    memset(symbol, 0, sizeof(symbol));
    zs_json_get_string(zjson, "Symbol", symbol, sizeof(symbol));
    if (!symbol[0]) {
        // ERRORID: no symbol in strategy setting
        zs_log_error(zse->Log, "cta: strategy_create no 'Symbol' in setting");
        return NULL;
    }
#endif//0

    // not release the zjson, since strategy would get conf param
    strategy->SettingJson = zjson;

    strategy->pAccountID = ztl_pcalloc(zse->Pool, 24);
    strncpy(strategy->pAccountID, account_id, 15);
    strategy->pCustomID = ztl_pcalloc(zse->Pool, 24);
    strncpy(strategy->pCustomID, strategy_name, 15);
    strategy->pStrategyName = ztl_pcalloc(zse->Pool, 24);
    strncpy(strategy->pStrategyName, strategy_name, 15);

    strategy->Engine = zse;
    strategy->Entry = NULL;         // will be assigned outside
    strategy->Blotter = NULL;       // will be assigned outside
    strategy->AssetFinder = zse->AssetFinder;
    strategy->Log = zse->Log;

    strategy->lookup_sid        = zs_cta_lookup_sid;
    strategy->lookup_symbol     = zs_cta_lookup_symbol;
    strategy->subscribe         = zs_cta_subscribe;
    strategy->subscribe_batch   = zs_cta_subscribe_batch;

    strategy->order             = zs_cta_order;
    strategy->place_order       = zs_cta_place_order;
    strategy->cancel_order      = zs_cta_cancel;
    strategy->cancel_order2     = zs_cta_cancel_req;
    strategy->cancel_all        = zs_cta_cancel_all;
    strategy->get_account_position = zs_cta_get_account_position;
    strategy->get_trading_account = zs_cta_get_trading_account;
    strategy->get_open_orders   = zs_cta_get_open_orders;
    strategy->get_orders        = zs_cta_get_orders;
    strategy->get_trades        = zs_cta_get_trades;
    strategy->get_contract      = zs_cta_get_contract;

    strategy->get_trading_day   = zs_cta_get_trading_day;
    strategy->get_info          = zs_cta_strategy_get_info;
    strategy->write_log         = zs_cta_write_log;
    strategy->get_conf_val      = zs_cta_get_conf_val;

    return strategy;
}

void zs_cta_strategy_release(zs_cta_strategy_t* strategy)
{
    zs_log_info(strategy->Log, "cta: strategy_release name:%s,ptr:%p",
        strategy->pStrategyName, strategy);

    if (strategy)
    {
        if (strategy->SettingJson) {
            zs_json_release(strategy->SettingJson);
            strategy->SettingJson = NULL;
        }
    }
}

int zs_cta_strategy_set_trading_flag(zs_cta_strategy_t* strategy, ZSTradingFlag trading_flag)
{
    strategy->TradingFlag = trading_flag;
    return ZS_OK;
}

int zs_cta_strategy_print(zs_cta_strategy_t* strategy)
{
    // todo: print the strategy info
    return 0;
}

int32_t zs_cta_get_trading_day(zs_cta_strategy_t* strategy, int flag)
{
    // todo:
    if (flag == 0)
        return strategy->Blotter->TradingDay;
    return -1;
}

const char* zs_cta_strategy_get_info(zs_cta_strategy_t* strategy)
{
    // todo: make json data or structure data
    return NULL;
}

void zs_cta_strategy_set_entry(zs_cta_strategy_t* strategy, zs_strategy_entry_t* strategy_entry)
{
    strategy->Entry = strategy_entry;
}

void zs_cta_strategy_set_blotter(zs_cta_strategy_t* strategy, zs_blotter_t* blotter)
{
    strategy->Blotter = blotter;
}


zs_sid_t zs_cta_lookup_sid(zs_cta_strategy_t* strategy, ZSExchangeID exchangeid, const char* symbol, int len)
{
    zs_contract_t*  contract;

    contract = zs_asset_find(strategy->Engine->AssetFinder, exchangeid, symbol, len);
    if (!contract) {
        return ZS_SID_INVALID;
    }

    return contract->Sid;
}

const char* zs_cta_lookup_symbol(zs_cta_strategy_t* strategy, zs_sid_t sid, ZSExchangeID* pexchangeid)
{
    zs_contract_t*  contract;

    contract = zs_asset_find_by_sid(strategy->Engine->AssetFinder, sid);
    if (!contract) {
        return NULL;
    }

    if (pexchangeid)
        *pexchangeid = contract->ExchangeID;
    return contract->Symbol;
}

int zs_cta_subscribe(zs_cta_strategy_t* strategy, zs_sid_t sid)
{
    return zs_strategy_subscribe_bysid(strategy->Engine, strategy, sid);
}

int zs_cta_subscribe_batch(zs_cta_strategy_t* strategy, zs_sid_t sids[], int count)
{
    return zs_strategy_subscribe_bysid_batch(strategy->Engine, strategy, sids, count);
}

int zs_cta_order(zs_cta_strategy_t* strategy, zs_sid_t sid, int order_qty, double order_price, ZSDirection direction, ZSOffsetFlag offset)
{
    zs_position_engine_t*   pos_engine;
    zs_contract_t*          contract;
    zs_order_req_t          order_req;
    memset(&order_req, 0, sizeof(order_req));

    contract = zs_asset_find_by_sid(strategy->Engine->AssetFinder, sid);
    if (!contract) {
        return ZS_ERR_NoContract;
    }

    if (offset != ZS_OF_Open)
    {
        pos_engine = NULL;
        zs_cta_get_account_position(strategy, &pos_engine, sid);
        if (!pos_engine) {
            return ZS_ERR_NoPosEngine;
        }

        if (direction == ZS_D_Long)
        {
            if (pos_engine->ShortTdPos - pos_engine->ShortTdFrozen > 0) {
                offset = ZS_OF_CloseToday;
            }
            else {
                offset = ZS_OF_Close;
            }
        }
        else if (direction == ZS_D_Short)
        {
            if (pos_engine->LongTdPos - pos_engine->LongTdFrozen > 0) {
                offset = ZS_OF_CloseToday;
            }
            else {
                offset = ZS_OF_Close;
            }
        }
        else {
            return ZS_ERR_FieldDirection;
        }
    }

    strcpy(order_req.Symbol, contract->Symbol);
    strcpy(order_req.AccountID, strategy->pAccountID);
    strcpy(order_req.UserID, strategy->pCustomID);

    order_req.ExchangeID    = contract->ExchangeID;
    order_req.Sid           = sid;
    order_req.OrderQty      = order_qty;
    order_req.OrderPrice    = order_price;
    order_req.Direction     = direction;
    order_req.OffsetFlag    = offset;
    order_req.OrderType     = order_price < 0.001 ? ZS_OT_Market : ZS_OT_Limit;

    // set this member for easily process internally
    order_req.Contract  = contract;

    return zs_cta_place_order(strategy, &order_req);
}

int zs_cta_place_order(zs_cta_strategy_t* strategy, zs_order_req_t* order_req)
{
    int rv;

    // 策略未运行
    if (strategy->RunStatus != ZS_RS_Running) {
        return ZS_ERR_NotStarted;
    }

    // 策略处于禁止交易/禁止开仓状态
    if (strategy->TradingFlag == ZS_TF_Paused) {
        return ZS_ERR_PlacePaused;
    }
    else if (strategy->TradingFlag == ZS_TF_PauseOpen) {
        return ZS_ERR_PlaceOpenPaused;
    }

    // 执行下单
    rv = zs_blotter_order(strategy->Blotter, order_req);
    if (rv == ZS_OK)
    {
        zs_strategy_engine_save_order(strategy->Engine, strategy, order_req);
    }
    return rv;
}

int zs_cta_cancel(zs_cta_strategy_t* strategy, int frontid, int sessionid, const char* orderid)
{
    zs_order_t* order;
    order = zs_cta_get_order(strategy, frontid, sessionid, orderid);
    if (order)
    {
        return zs_cta_cancelex(strategy, order);
    }

    return ZS_ERR_NoOrder;
}

int zs_cta_cancel_by_sysid(zs_cta_strategy_t* strategy, ZSExchangeID exchangeid, const char* order_sysid)
{
    zs_order_t* order;
    order = zs_cta_get_order_by_sysid(strategy, exchangeid, order_sysid);
    if (order)
    {
        return zs_cta_cancelex(strategy, order);
    }

    return ZS_ERR_NoOrder;
}

int zs_cta_cancel_req(zs_cta_strategy_t* strategy, zs_cancel_req_t* cancel_req)
{
    return zs_blotter_cancel(strategy->Blotter, cancel_req);
}

int zs_cta_cancelex(zs_cta_strategy_t* strategy, zs_order_t* order)
{
    int rv;
    if (!order) {
        return ZS_ERROR;
    }

    if (is_finished_status(order->OrderStatus)) {
        // ERRORID: already finished
        return ZS_ERR_OrderFinished;
    }

    zs_cancel_req_t cancel_req;
    memset(&cancel_req, 0, sizeof(zs_cancel_req_t));

    strcpy(cancel_req.AccountID, order->AccountID);
    strcpy(cancel_req.BrokerID, order->BrokerID);
    strcpy(cancel_req.Symbol, order->Symbol);
    strcpy(cancel_req.OrderSysID, order->OrderSysID);
    strcpy(cancel_req.OrderID, order->OrderID);
    cancel_req.FrontID = order->FrontID;
    cancel_req.SessionID = order->SessionID;
    cancel_req.ExchangeID = order->ExchangeID;

    rv = zs_cta_cancel_req(strategy, &cancel_req);
    return rv;
}

int zs_cta_cancel_all(zs_cta_strategy_t* strategy)
{
    int count;
    zs_order_t* open_orders[1020] = { 0 };
    count = zs_cta_get_open_orders(strategy, open_orders, 1020, ZS_SID_WILDCARD);
    zs_log_info(strategy->Log, "cta: cancel_all count:%d", count);

    for (int i = 0; i < count; ++i)
    {
        zs_cta_cancelex(strategy, open_orders[i]);
    }

    return ZS_OK;
}


int zs_cta_get_account_position(zs_cta_strategy_t* strategy, zs_position_engine_t** ppos_engine, zs_sid_t sid)
{
    zs_position_engine_t* pos_engine;
    pos_engine = zs_position_engine_get(strategy->Blotter, sid);
    if (pos_engine)
    {
        *ppos_engine = pos_engine;
        return ZS_OK;
    }

    return ZS_ERR_NoPosEngine;
}

int zs_cta_get_strategy_position(zs_cta_strategy_t* strategy, zs_position_engine_t** ppos_engine, zs_sid_t sid)
{
    // ERRORID: not support currently
    *ppos_engine = strategy->PosEngine;
    return ZS_ERR_NotImpl;
}

int zs_cta_get_trading_account(zs_cta_strategy_t* strategy, zs_account_t** paccount)
{
    *paccount = strategy->Blotter->Account;
    return ZS_OK;
}


int zs_cta_get_trades(zs_cta_strategy_t* strategy, 
    zs_trade_t* trades[], int size, zs_sid_t filter_sid)
{
    int32_t index, count;
    zs_trade_t* trade;

    index = 0;
    count = 0;

    for (; index < (int32_t)ztl_array_size(strategy->Blotter->TradeArray); ++index)
    {
        trade = (zs_trade_t*)ztl_array_at2(strategy->Blotter->TradeArray, index);
        if (index >= size) {
            break;
        }

        if (!filter_sid || trade->Sid == filter_sid)
            trades[count++] = trade;
    }

    return count;
}

int zs_cta_get_open_orders(zs_cta_strategy_t* strategy,
    zs_order_t* open_orders[], int size, zs_sid_t filter_sid)
{
    return zs_orderlist_retrieve(strategy->Blotter->WorkOrderList,
        open_orders, size, filter_sid);
}

int zs_cta_get_orders(zs_cta_strategy_t* strategy,
    zs_order_t* orders[], int size, zs_sid_t filter_sid)
{
    ztl_dict_t*     dict;
    dictIterator*   iter;
    dictEntry*      entry;
    zs_order_t*     order;
    int             index;

    dict = strategy->Blotter->OrderDict;
    iter = dictGetIterator(dict);
    index = 0;

    while (true)
    {
        entry = dictNext(iter);
        if (!entry) {
            break;
        }

        order = (zs_order_t*)entry->v.val;
        if (!filter_sid || order->Sid == filter_sid) {
            orders[index++] = order;
        }
    }

    dictReleaseIterator(iter);

    return index;
}

zs_order_t* zs_cta_get_order(zs_cta_strategy_t* strategy,
    int frontid, int sessionid, const char* orderid)
{
    return zs_get_order_by_id(strategy->Blotter, frontid, sessionid, orderid);
}

zs_order_t* zs_cta_get_order_by_sysid(zs_cta_strategy_t* strategy,
    ZSExchangeID exchangeid, const char* order_sysid)
{
    return zs_get_order_by_sysid(strategy->Blotter, exchangeid, order_sysid);
}

zs_contract_t* zs_cta_get_contract(zs_cta_strategy_t* strategy, zs_sid_t sid)
{
    zs_contract_t*  contract;
    contract = zs_asset_find_by_sid(strategy->Engine->AssetFinder, sid);
    return contract;
}


// deprecated
static int _make_log_line_time(char* buf)
{
    int lLength = 0;
    lLength += ztl_hmsu(buf + lLength);
    lLength += sprintf(buf + lLength, " [%u] ", (uint32_t)ztl_thread_self());
    return lLength;
}

int zs_cta_write_log(zs_cta_strategy_t* strategy, const char* content, ...)
{
    int  length;
    char buffer[4090];

    length = 0;
    // length += _make_log_line_time(buffer + length);

    va_list args;
    va_start(args, content);
    length += vsnprintf(buffer + length, 4090 - length - 2, content, args);
    va_end(args);

    // append line feed
//     buffer[length] = '\r';
//     buffer[length + 1] = '\n';
//     length += 2;
//     buffer[length] = '\0';

    // fprintf(stderr, buffer);
    zs_log_info(strategy->Log, buffer);

    return 0;
}

int zs_cta_get_conf_val(zs_cta_strategy_t* strategy, 
    const char* key, void* val, int size, ZSCType ctype)
{
    zs_json_t* zjson;
    zjson = strategy->SettingJson;

    if (!zjson) {
        return ZS_ERR_JsonData;
    }

    if (!zs_json_have_object(zjson, key)) {
        return ZS_ERR_NoConfItem;
    }

    switch (ctype)
    {
    case ZS_CT_Int32:
        return zs_json_get_int(zjson, key, val);
    case ZS_CT_Double:
        return zs_json_get_double(zjson, key, val);
    case ZS_CT_String:
        return zs_json_get_string(strategy->SettingJson, key, val, size);
    default:
        return ZS_ERROR;
    }
}

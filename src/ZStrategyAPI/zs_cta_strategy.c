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


zs_sid_t zs_cta_lookup_sid(zs_cta_strategy_t* context, ZSExchangeID exchangeid, const char* symbol, int len);
const char* zs_cta_lookup_symbol(zs_cta_strategy_t* context, zs_sid_t sid);
int zs_cta_subscribe(zs_cta_strategy_t* context, zs_sid_t sid);

int zs_cta_order(zs_cta_strategy_t* strategy, zs_sid_t sid, int order_qty, double order_price, ZSDirection direction, ZSOffsetFlag offset);
int zs_cta_place_order(zs_cta_strategy_t* strategy, zs_order_req_t* order_req);
int zs_cta_cancel(zs_cta_strategy_t* strategy, zs_cancel_req_t* cancel_req);
int zs_cta_cancelex(zs_cta_strategy_t* strategy, zs_order_t* order);
int zs_cta_cancel_all(zs_cta_strategy_t* strategy);

int zs_cta_get_account_position(zs_cta_strategy_t* context, zs_position_engine_t** ppos_engine, zs_sid_t sid);
int zs_cta_get_strategy_position(zs_cta_strategy_t* context, zs_position_engine_t** ppos_engine, zs_sid_t sid);
int zs_cta_get_trading_account(zs_cta_strategy_t* context, zs_account_t** paccount);
int zs_cta_get_open_orders(zs_cta_strategy_t* context, zs_order_t* open_orders[], int size);
int zs_cta_get_orders(zs_cta_strategy_t* context, zs_order_t* orders[], int size);
int zs_cta_get_trades(zs_cta_strategy_t* context, zs_trade_t* trades[], int size);
zs_contract_t* zs_cta_get_contract(zs_cta_strategy_t* context, zs_sid_t sid);

int zs_cta_strategy_print(zs_cta_strategy_t* strategy);
int32_t zs_cta_get_trading_day(zs_cta_strategy_t* strategy, int flag);
const char* zs_cta_strategy_get_info(zs_cta_strategy_t* strategy);
int zs_cta_write_log(zs_cta_strategy_t* context, const char* content, ...);
int zs_cta_get_conf_val(zs_cta_strategy_t* context, const char* key, void* val, int size, ZSCType ctype);



zs_cta_strategy_t* zs_cta_strategy_create(zs_strategy_engine_t* engine, const char* setting, uint32_t strategy_id)
{
    zs_json_t* zjson;
    zs_cta_strategy_t* strategy;
    char strategy_name[16];
    char account_id[16];
    char symbol[32];

    strategy = ztl_pcalloc(engine->Pool, sizeof(zs_cta_strategy_t));

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
        return NULL;
    }

    memset(symbol, 0, sizeof(symbol));
    zs_json_get_string(zjson, "Symbol", symbol, sizeof(symbol));
    if (!symbol[0]) {
        // ERRORID: no symbol in strategy setting
        return NULL;
    }

    // not release the zjson, since strategy would get conf param
    strategy->SettingJson = zjson;

    strategy->pAccountID = ztl_pcalloc(engine->Pool, 16);
    strcpy(strategy->pAccountID, account_id);
    strategy->pCustomID = ztl_pcalloc(engine->Pool, 16);
    strcpy(strategy->pCustomID, strategy_name);

    strategy->Engine = engine;
    strategy->Entry = NULL;         // will be assigned outside
    strategy->Blotter = NULL;       // will be assigned outside
    strategy->AssetFinder = engine->AssetFinder;

    strategy->lookup_sid = zs_cta_lookup_sid;
    strategy->lookup_symbol = zs_cta_lookup_symbol;
    strategy->subscribe = zs_cta_subscribe;

    strategy->order = zs_cta_order;
    strategy->place_order = zs_cta_place_order;
    strategy->cancel_order = zs_cta_cancel;
    strategy->cancel_all = zs_cta_cancel_all;
    strategy->get_account_position = zs_cta_get_account_position;
    strategy->get_trading_account = zs_cta_get_trading_account;
    strategy->get_open_orders = zs_cta_get_open_orders;
    strategy->get_orders = zs_cta_get_orders;
    strategy->get_trades = zs_cta_get_trades;
    strategy->get_contract = zs_cta_get_contract;

    strategy->get_trading_day = zs_cta_get_trading_day;
    strategy->get_info = zs_cta_strategy_get_info;
    strategy->write_log = zs_cta_write_log;
    strategy->get_conf_val = zs_cta_get_conf_val;

    return strategy;
}

void zs_cta_strategy_release(zs_cta_strategy_t* strategy)
{
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
    return 0;
}

int zs_cta_strategy_print(zs_cta_strategy_t* strategy)
{
    // todo: print the strategy info
    return 0;
}

int32_t zs_cta_get_trading_day(zs_cta_strategy_t* strategy, int flag)
{
    // todo:
    return 0;
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

const char* zs_cta_lookup_symbol(zs_cta_strategy_t* strategy, zs_sid_t sid)
{
    zs_contract_t*  contract;

    contract = zs_asset_find_by_sid(strategy->Engine->AssetFinder, sid);
    if (!contract) {
        return NULL;
    }

    return contract->Symbol;
}

int zs_cta_subscribe(zs_cta_strategy_t* strategy, zs_sid_t sid)
{
    return zs_strategy_subscribe_bysid(strategy->Engine, strategy, sid);
}

int zs_cta_order(zs_cta_strategy_t* strategy, zs_sid_t sid, int order_qty, double order_price, ZSDirection direction, ZSOffsetFlag offset)
{
    zs_contract_t*  contract;
    zs_order_req_t  order_req;
    memset(&order_req, 0, sizeof(order_req));

    contract = zs_asset_find_by_sid(strategy->Engine->AssetFinder, sid);
    if (!contract) {
        // ERRORID: not find the contract info
        return -1;
    }

    strcpy(order_req.Symbol, contract->Symbol);
    strcpy(order_req.AccountID, strategy->pAccountID);
    strcpy(order_req.UserID, strategy->pCustomID);

    order_req.ExchangeID = contract->ExchangeID;
    order_req.Sid       = sid;
    order_req.OrderQty  = order_qty;
    order_req.OrderPrice= order_price;
    order_req.Direction = direction;
    order_req.OffsetFlag= offset;
    order_req.OrderType = order_price < 0.001 ? ZS_OT_Market : ZS_OT_Limit;

    // set this member for easily process internally
    order_req.Contract  = contract;

    return zs_cta_place_order(strategy, &order_req);
}

int zs_cta_place_order(zs_cta_strategy_t* strategy, zs_order_req_t* order_req)
{
    int rv;
    rv = zs_blotter_order(strategy->Blotter, order_req);
    if (rv == 0)
    {
        zs_strategy_engine_save_order(strategy->Engine, strategy, order_req);
    }
    return rv;
}

int zs_cta_cancel(zs_cta_strategy_t* strategy, zs_cancel_req_t* cancel_req)
{
    return zs_blotter_cancel(strategy->Blotter, cancel_req);
}

int zs_cta_cancelex(zs_cta_strategy_t* strategy, zs_order_t* order)
{
    int rv;
    if (!order) {
        return -1;
    }

    if (is_finished_status(order->OrderStatus))
    {
        // ERRORID: already finished
        return -2;
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

    rv = zs_cta_cancel(strategy, &cancel_req);
    return rv;
}

int zs_cta_cancel_all(zs_cta_strategy_t* strategy)
{
    int count;
    zs_order_t* open_orders[1020];
    count = zs_cta_get_open_orders(strategy, open_orders, 1020);

    for (int i = 0; i < count; ++i)
    {
        zs_cta_cancelex(strategy, open_orders[i]);
    }

    return 0;
}


int zs_cta_get_account_position(zs_cta_strategy_t* strategy, zs_position_engine_t** ppos_engine, zs_sid_t sid)
{
    zs_position_engine_t* pos_engine;
    pos_engine = zs_position_engine_get(strategy->Blotter, sid);
    if (pos_engine)
    {
        *ppos_engine = pos_engine;
        return 0;
    }

    return -1;
}

int zs_cta_get_strategy_position(zs_cta_strategy_t* context, zs_position_engine_t** ppos_engine, zs_sid_t sid)
{
    // ERRORID: not support currently
    return -1;
}

int zs_cta_get_trading_account(zs_cta_strategy_t* strategy, zs_account_t** paccount)
{
    *paccount = strategy->Blotter->Account;
    return 0;
}

int zs_cta_get_open_orders(zs_cta_strategy_t* strategy, zs_order_t* open_orders[], int size)
{
    return zs_orderlist_retrieve(strategy->Blotter->WorkOrderList, open_orders, size);
}

int zs_cta_get_orders(zs_cta_strategy_t* strategy, zs_order_t* orders[], int size)
{
    ztl_dict_t*     dict;
    dictIterator*   iter;
    dictEntry*      entry;
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

        orders[index++] = entry->v.val;
    }

    dictReleaseIterator(iter);

    return index;
}

int zs_cta_get_trades(zs_cta_strategy_t* strategy, zs_trade_t* trades[], int size)
{
    int32_t index;
    zs_trade_t* trade;

    for (index = 0; index < (int32_t)ztl_array_size(strategy->Blotter->TradeArray); ++index)
    {
        trade = (zs_trade_t*)ztl_array_at2(strategy->Blotter->TradeArray, index);
        if (index >= size) {
            break;
        }

        trades[index] = trade;
    }
    return 0;
}

zs_contract_t* zs_cta_get_contract(zs_cta_strategy_t* strategy, zs_sid_t sid)
{
    zs_contract_t*  contract;
    contract = zs_asset_find_by_sid(strategy->Engine->AssetFinder, sid);
    return contract;
}


static int _make_log_line_time(char* buf)
{
    int lLength = 0;
    lLength += ztl_hmsu(buf + lLength);
    lLength += sprintf(buf + lLength, " [%u] ", (uint32_t)ztl_thread_self());
    return lLength;
}

int zs_cta_write_log(zs_cta_strategy_t* context, const char* content, ...)
{
    int  length;
    char buffer[4090];

    length = 0;
    length += _make_log_line_time(buffer + length);

    va_list args;
    va_start(args, content);
    length += vsnprintf(buffer + length, 4090 - length - 2, content, args);
    va_end(args);

    // append line feed
    buffer[length] = '\r';
    buffer[length + 1] = '\n';
    length += 2;
    buffer[length] = '\0';

    fprintf(stderr, buffer);

    return 0;
}

int zs_cta_get_conf_val(zs_cta_strategy_t* strategy, 
    const char* key, void* val, int size, ZSCType ctype)
{
    zs_json_t* zjson;
    zjson = strategy->SettingJson;

    if (!zjson) {
        return -1;
    }

    if (!zs_json_have_object(zjson, key)) {
        return -2;
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
        return -1;
    }
}

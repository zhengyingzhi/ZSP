#include <ZToolLib/ztl_atomic.h>
#include <ZToolLib/ztl_times.h>
#include <ZToolLib/ztl_utils.h>


#ifdef _MSC_VER
#include <Windows.h>
#endif

#include "zs_error.h"
#include "zs_slippage.h"


// volume limit for matching order by bar
#define ZS_DEFAULT_VOLUME_LIMIT     0.025


static bool zs_fill_worse_than_limit(double filled_price, 
    const zs_order_req_t* order_req);

static int zs_order_cmp(void* expect, void* actual);

static void zs_generate_order(zs_order_t* order, const zs_order_req_t* order_req, int filled_qty);
static void zs_update_order(zs_order_t* order, double filled_price, int filled_qty);
static void zs_generate_trade(zs_trade_t* trade, const zs_order_t* order, 
    double filled_price, int filled_qty);


static int _zs_process_order_bar_volume_share(
    zs_slippage_model_t* slippage_model, 
    zs_bar_reader_t* bar_reader,
    const zs_order_t* order, 
    double* pfilled_price, int* pfilled_qty);

static int _zs_process_order_tick(
    zs_slippage_model_t* slippage_model,
    zs_tick_t* tick,
    const zs_order_t* order,
    double* pfilled_price, int* pfilled_qty);


/* Model slippage as a function of the volume of contracts traded */
static zs_slippage_model_t ZSVolumeShareSlippage = {
    "VolumeShareSlippage",
    NULL,
    ZS_PFF_Open,
    ZS_DEFAULT_VOLUME_LIMIT,
    0,
    _zs_process_order_bar_volume_share,
    _zs_process_order_tick
};


static int _zs_next_order_sysid(zs_slippage_t* slippage, char order_sysid[])
{
    uint32_t sysid = ztl_atomic_add(&slippage->order_sysid, 1);
    return sprintf(order_sysid, "%11u", sysid);
}

static int _zs_next_trade_id(zs_slippage_t* slippage, char trade_id[])
{
    uint32_t tid = ztl_atomic_add(&slippage->trade_id, 1);
    return sprintf(trade_id, "%u", tid);
}


zs_slippage_t* zs_slippage_create(zs_slippage_handler_pt handler, void* userdata)
{
    zs_slippage_t* slippage;

    slippage = (zs_slippage_t*)calloc(1, sizeof(zs_slippage_t));

    slippage->OrderList     = ztl_dlist_create(32);
    slippage->NewOrders     = ztl_dlist_create(32);
    slippage->CancelRequests= ztl_dlist_create(32);

    // entity_size will be adjusted to zs_quote_order_t
    int entity_size         = ztl_align(sizeof(zs_order_t), 8);
    int init_count          = 64;
    slippage->MemPool       = ztl_mp_create(entity_size, init_count, 1);

    slippage->SlippageModel = &ZSVolumeShareSlippage;
    slippage->Handler       = handler;
    slippage->UserData      = userdata;
    slippage->PriceField    = ZS_PFF_Open;

    slippage->order_sysid   = 1;
    slippage->trade_id      = 1;

    return slippage;
}

void zs_slippage_release(zs_slippage_t* slippage)
{
    if (slippage)
    {
        if (slippage->OrderList) {
            ztl_dlist_release(slippage->OrderList);
            slippage->OrderList = NULL;
        }

        if (slippage->NewOrders) {
            ztl_dlist_release(slippage->NewOrders);
            slippage->NewOrders = NULL;
        }

        if (slippage->CancelRequests) {
            ztl_dlist_release(slippage->CancelRequests);
            slippage->CancelRequests = NULL;
        }

        if (slippage->MemPool) {
            ztl_mp_release(slippage->MemPool);
            slippage->MemPool = NULL;
        }

        free(slippage);
    }
}

void zs_slippage_set_price_field(zs_slippage_t* slippage, ZS_PRICE_FIELD_FILL price_field)
{
    slippage->PriceField = price_field;
}

void zs_slippage_set_model(zs_slippage_t* slippage, zs_slippage_model_t* slippage_model)
{
    slippage->SlippageModel = slippage_model;
}

void zs_slippage_reset(zs_slippage_t* slippage)
{
    ztl_dlist_iterator_t* iter;

    iter = ztl_dlist_iter_new(slippage->OrderList, ZTL_DLSTART_HEAD);
    while (true)
    {
        zs_order_t* cur_order;
        cur_order = ztl_dlist_next(slippage->OrderList, iter);
        if (!cur_order) {
            break;
        }
        ztl_mp_free(slippage->MemPool, cur_order);
    }
    ztl_dlist_iter_del(slippage->OrderList, iter);

    iter = ztl_dlist_iter_new(slippage->NewOrders, ZTL_DLSTART_HEAD);
    while (true)
    {
        zs_order_t* cur_order;
        cur_order = ztl_dlist_next(slippage->NewOrders, iter);
        if (!cur_order) {
            break;
        }
        ztl_mp_free(slippage->MemPool, cur_order);
    }
    ztl_dlist_iter_del(slippage->NewOrders, iter);

    iter = ztl_dlist_iter_new(slippage->CancelRequests, ZTL_DLSTART_HEAD);
    while (true)
    {
        zs_cancel_req_t* cancel_req;
        cancel_req = ztl_dlist_next(slippage->CancelRequests, iter);
        if (!cancel_req) {
            break;
        }
        ztl_mp_free(slippage->MemPool, cancel_req);
    }
    ztl_dlist_iter_del(slippage->CancelRequests, iter);

    if (ztl_mp_exposed(slippage->MemPool) > 1) {
        // error: not all data was released
    }
}

int zs_slippage_update_tradingday(zs_slippage_t* slippage, int32_t trading_day)
{
    if (slippage->trading_day == trading_day) {
        return ZS_EXISTED;
    }
    else if (slippage->trading_day < trading_day) {
        return ZS_ERROR;
    }

    // reset some data
    zs_slippage_reset(slippage);

    return ZS_OK;
}

int zs_slippage_order(zs_slippage_t* slippage, 
    const zs_order_req_t* order_req)
{
    zs_order_t* order;
    order = (zs_order_t*)ztl_mp_alloc(slippage->MemPool);
    _zs_next_order_sysid(slippage, order->OrderSysID);
    zs_generate_order(order, order_req, 0);

    // add to new order firstly
    ztl_dlist_insert_tail(slippage->NewOrders, order);

    return 0;
}

int zs_slippage_quote_order(zs_slippage_t* slippage, 
    const zs_quote_order_req_t* quoteOrderReq)
{
    return ZS_ERR_NotImpl;
}


int zs_slippage_cancel(zs_slippage_t* slippage,
    const zs_cancel_req_t* cancel_req)
{
    zs_cancel_req_t* cancel_req_dup;
    cancel_req_dup = (zs_cancel_req_t*)ztl_mp_alloc(slippage->MemPool);
    *cancel_req_dup = *cancel_req;

    ztl_dlist_insert_tail(slippage->CancelRequests, cancel_req_dup);
    return ZS_OK;
}

int zs_slippage_process_cancel(zs_slippage_t* slippage, 
    zs_cancel_req_t* cancel_req)
{
    int rv;
    ztl_dlist_t* order_list;
    ztl_dlist_iterator_t* iter;

    order_list = slippage->OrderList;
    iter = ztl_dlist_iter_new(order_list, ZTL_DLSTART_HEAD);
    while (true)
    {
        zs_order_t* cur_order;
        cur_order = ztl_dlist_next(order_list, iter);
        if (!cur_order) {
            rv = ZS_ERROR;
            break;
        }

        if ((strcmp(cur_order->OrderSysID, cancel_req->OrderSysID) == 0 &&
            cur_order->ExchangeID == cancel_req->ExchangeID)
            || (cur_order->OrderID == cancel_req->OrderID && 
                cur_order->FrontID == cancel_req->FrontID && 
                cur_order->SessionID == cancel_req->SessionID))
        {
            // found
            cur_order->OrderStatus = ZS_OS_Canceld;
            if (cur_order->FilledQty > 0)
            {
                cur_order->OrderStatus = ZS_OS_PartCancled;
            }
            cur_order->CancelTime = (int32_t)(ztl_intdatetime() % 100000000);

            // order rtn
            slippage->Handler(slippage, ZS_SDT_Order, cur_order, sizeof(*cur_order));

            // erase
            ztl_dlist_erase(order_list, iter);
            ztl_mp_free(slippage->MemPool, cur_order);

            rv = ZS_OK;
            break;
        }
    }
    ztl_dlist_iter_del(order_list, iter);
    return rv;
}


static void zs_slippage_process_neworder(zs_slippage_t* slippage)
{
    // 新订单先发送一个确认
    ztl_dlist_t*    new_orders;
    zs_order_t*     cur_order;
    ztl_dlist_iterator_t* iter;

    new_orders = slippage->NewOrders;
    if (ztl_dlist_size(new_orders) == 0) {
        return;
    }

    iter = ztl_dlist_iter_new(new_orders, ZTL_DLSTART_HEAD);
    while (true)
    {
        cur_order = ztl_dlist_next(new_orders, iter);
        if (!cur_order) {
            break;
        }

        slippage->Handler(slippage, ZS_SDT_Order, cur_order, sizeof(*cur_order));

        // append to order list
        ztl_dlist_insert_tail(slippage->OrderList, cur_order);

        ztl_dlist_erase(new_orders, iter);
    }
    ztl_dlist_iter_del(new_orders, iter);
}

int zs_slippage_process_order(zs_slippage_t* slippage, 
    zs_bar_reader_t* bar_reader, zs_tick_t* tick)
{
    int     rv;
    int     filled_qty;
    double  filled_price;

    ztl_dlist_t*     order_list;
    zs_order_t*      cur_order;
    zs_cancel_req_t* cancel_req;
    ztl_dlist_iterator_t* iter;

    // process pending queue firstly
    zs_slippage_process_neworder(slippage);

    if (ztl_dlist_size(slippage->CancelRequests) > 0)
    {
        iter = ztl_dlist_iter_new(slippage->CancelRequests, ZTL_DLSTART_HEAD);
        while (true)
        {
            cancel_req = ztl_dlist_next(slippage->CancelRequests, iter);
            if (!cancel_req) {
                break;
            }

            if (zs_slippage_process_cancel(slippage, cancel_req) == ZS_OK)
            {
                ztl_dlist_erase(slippage->CancelRequests, iter);
                ztl_mp_free(slippage->MemPool, cancel_req);
            }
        }
        ztl_dlist_iter_del(slippage->CancelRequests, iter);
    }

    // no pending order
    if (ztl_dlist_size(slippage->OrderList) == 0) {
        return ZS_OK;
    }

    order_list = slippage->OrderList;
    iter = ztl_dlist_iter_new(order_list, ZTL_DLSTART_HEAD);
    while (true)
    {
        filled_qty = 0;
        filled_price = 0.0;

        cur_order = ztl_dlist_next(order_list, iter);
        if (!cur_order) {
            rv = ZS_OK;
            break;
        }

        // get the order filled price & volume
        if (bar_reader)
        {
            if (!bar_reader->DataPortal && bar_reader->Bar.Sid != cur_order->Sid) {
                continue;
            }

            rv = slippage->SlippageModel->process_order_by_bar(
                slippage->SlippageModel, bar_reader, cur_order, 
                &filled_price, &filled_qty);
        }
        else if (tick)
        {
            if (tick->Sid != cur_order->Sid) {
                continue;
            }

            rv = slippage->SlippageModel->process_order_by_tick(
                slippage->SlippageModel, tick, cur_order,
                &filled_price, &filled_qty);
        }
        else {
            rv = ZS_ERROR;
            break;
        }

        // continue for next order
        if (rv != ZS_OK) {
            continue;
        }

        // order rtn
        zs_update_order(cur_order, filled_price, filled_qty);
        slippage->Handler(slippage, ZS_SDT_Order, cur_order, sizeof(*cur_order));

        // trade rtn
        zs_trade_t trade = { 0 };
        _zs_next_trade_id(slippage, trade.TradeID);
        zs_generate_trade(&trade, cur_order, filled_price, filled_qty);
        slippage->Handler(slippage, ZS_SDT_Trade, &trade, sizeof(trade));

        if (cur_order->OrderStatus == ZS_OS_Filled)
        {
            ztl_dlist_erase(order_list, iter);

            ztl_mp_free(slippage->MemPool, cur_order);
        }
    }
    ztl_dlist_iter_del(order_list, iter);

    return rv;
}

int zs_slippage_process_bybar(zs_slippage_t* slippage, zs_bar_t* bar)
{
    zs_bar_reader_t bar_reader = { 0 };
    bar_reader.Bar = *bar;
    return zs_slippage_process_order(slippage, &bar_reader, NULL);
}

int zs_slippage_process_bytick(zs_slippage_t* slippage, zs_tick_t* tick)
{
    return zs_slippage_process_order(slippage, NULL, tick);
}


static void zs_generate_order(zs_order_t* order, const zs_order_req_t* order_req, int filled_volume)
{
    strcpy(order->Symbol, order_req->Symbol);
    strcpy(order->BrokerID, order_req->BrokerID);
    strcpy(order->AccountID, order_req->AccountID);
    strcpy(order->UserID, order_req->UserID);
    strcpy(order->OrderID, order_req->OrderID);
    order->ExchangeID   = order_req->ExchangeID;
    order->Sid          = order_req->Sid;
    order->OrderPrice   = order_req->OrderPrice;
    order->OrderQty     = order_req->OrderQty;
    order->FilledQty    = filled_volume;

    order->Direction    = order_req->Direction;
    order->OffsetFlag   = order_req->OffsetFlag;
    order->OrderType    = order_req->OrderType;

    order->OrderStatus  = ZS_OS_Accepted;

    if ((filled_volume + order->FilledQty) < order_req->OrderQty) {
        order->OrderStatus = ZS_OS_PartFilled;
    }
    else if ((filled_volume + order->FilledQty) == order_req->OrderQty) {
        order->OrderStatus = ZS_OS_Filled;
    }

    int64_t dt = ztl_intdatetime();
    order->OrderDate = (int32_t)(dt / 100000000);
    order->OrderTime = (int32_t)(dt % 100000000);

    order->FrontID = 1;
    order->SessionID = 1;
}

static void zs_update_order(zs_order_t* order, double filled_price, int filled_qty)
{
    if (filled_qty == 0 && order->FilledQty == 0) {
        order->OrderStatus = ZS_OS_Accepted;
    }
    else
    {
        order->AvgPrice = ((order->FilledQty * order->AvgPrice) + filled_qty * filled_price) / (order->FilledQty + filled_qty);
        order->FilledQty += filled_qty;
        if ((filled_qty + order->FilledQty) < order->OrderQty) {
            order->OrderStatus = ZS_OS_PartFilled;
        }
        else {
            order->OrderStatus = ZS_OS_Filled;
        }
    }
}

static void zs_generate_trade(zs_trade_t* trade, const zs_order_t* order, double filled_price, int filled_qty)
{
    strcpy(trade->Symbol,       order->Symbol);
    strcpy(trade->BrokerID,     order->BrokerID);
    strcpy(trade->AccountID,    order->AccountID);
    strcpy(trade->UserID,       order->UserID);
    strcpy(trade->OrderSysID,   order->OrderSysID);
    strcpy(trade->OrderID,      order->OrderID);

    trade->ExchangeID   = order->ExchangeID;
    trade->Price        = filled_price;
    trade->Volume       = filled_qty;
    trade->Direction    = order->Direction;
    trade->OffsetFlag   = order->OffsetFlag;

    trade->Sid          = order->Sid;
    trade->TradingDay   = order->TradingDay;
    trade->TradeDate    = order->OrderDate;    // FIXME

    int64_t dt = ztl_intdatetime();
    trade->TradeTime = (int32_t)(dt % 100000000);
}

/////////////////////////////////////////////////////////////////////////////////////

/* Checks whether the fill price is worse than the order's limit price */
static bool zs_fill_worse_than_limit(double filled_price,
    const zs_order_req_t* order_req)
{
    return false;
}

static int zs_order_cmp(void* expect, void* actual)
{
    if (expect == actual) {
        return 0;
    }
    return -1;
}

static int _zs_process_order_bar_volume_share(
    zs_slippage_model_t* slippage_model,
    zs_bar_reader_t* bar_reader,
    const zs_order_t* order, 
    double* pfilled_price, int* pfilled_qty)
{
    // fill order by bar data
    zs_bar_t* bar;
    bar = bar_reader->current_bar(bar_reader, order->Sid);
    if (!bar || bar->Volume == 0)
    {
        return -1;
    }

    double open_px = bar->OpenPrice;
    double close_px = bar->ClosePrice;
    double price;
    if (slippage_model->PriceField == ZS_PFF_Close)
        price = close_px;
    else
        price = open_px;

    if (order->Direction == ZS_D_Long)
    {
        if (price >= order->OrderPrice)
        {
            *pfilled_price = price;
            *pfilled_qty = order->OrderQty;
            return ZS_OK;
        }
    }
    else if (order->Direction == ZS_D_Short)
    {
        if (price <= order->OrderPrice)
        {
            *pfilled_price = price;
            *pfilled_qty = order->OrderQty;
            return ZS_OK;
        }
    }

    return -2;
}

static int _zs_process_order_tick(
    zs_slippage_model_t* slippage_model,
    zs_tick_t* tick,
    const zs_order_t* order,
    double* pfilled_price, int* pfilled_qty)
{
    // fill order by tick data
    double last_px = tick->LastPrice;

    if (order->Direction == ZS_D_Long)
    {
        if (last_px >= order->OrderPrice)
        {
            *pfilled_price = last_px;
            *pfilled_qty = order->OrderQty;
            return 0;
        }
    }
    else if (order->Direction == ZS_D_Short)
    {
        if (last_px <= order->OrderPrice)
        {
            *pfilled_price = last_px;
            *pfilled_qty = order->OrderQty;
            return 0;
        }
    }
    return -2;
}

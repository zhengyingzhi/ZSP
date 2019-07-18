#include <ZToolLib/ztl_atomic.h>

#include <ZToolLib/ztl_times.h>

#include <ZToolLib/ztl_utils.h>


#ifdef _MSC_VER
#include <Windows.h>
#endif

#include "zs_slippage.h"


#define ZS_DEFAULT_EQUITY_VOLUME_LIMIT_FOR_BAR  0.025
#define ZS_DEFAULT_FUTURE_VOLUME_LIMIT_FOR_BAR  0.05

static uint32_t _zs_order_sys_id = 1;
static uint32_t _zs_trade_id = 1;


static bool zs_fill_worse_than_limit(double filled_price, 
    const zs_order_req_t* order_req);

static int zs_order_cmp(void* expect, void* actual);

static void zs_generate_order(zs_order_t* order, const zs_order_req_t* order_req, int filled_qty);
static void zs_update_order(zs_order_t* order, int filled_qty);
static void zs_generate_trade(zs_trade_t* trade, const zs_order_t* order, 
    double filled_price, int filled_qty);


static int _zs_process_order_volume_share(
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
    ZS_PFF_Close,
    ZS_DEFAULT_EQUITY_VOLUME_LIMIT_FOR_BAR,
    0,
    _zs_process_order_volume_share,
    _zs_process_order_tick
};


zs_slippage_t* zs_slippage_create(ZS_PRICE_FIELD_FILL price_field,
    zs_slippage_handler_pt handler, void* userdata)
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
    slippage->FillPriceField= price_field;

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

int zs_slippage_order(zs_slippage_t* slippage, 
    const zs_order_req_t* order_req)
{
    zs_order_t* order;
    order = (zs_order_t*)ztl_mp_alloc(slippage->MemPool);
    zs_generate_order(order, order_req, 0);

    // add to new order firstly
    ztl_dlist_insert_tail(slippage->NewOrders, order);

    return 0;
}

int zs_slippage_quote_order(zs_slippage_t* slippage, 
    const zs_quote_order_req_t* quoteOrderReq)
{
    return -1;
}


int zs_slippage_cancel(zs_slippage_t* slippage,
    const zs_cancel_req_t* cancelReq)
{
    zs_cancel_req_t* cancel_req;
    cancel_req = (zs_cancel_req_t*)ztl_mp_alloc(slippage->MemPool);
    *cancel_req = *cancel_req;

    ztl_dlist_insert_tail(slippage->CancelRequests, cancel_req);
    return 0;
}

int zs_slippage_process_cancel(zs_slippage_t* slippage, 
    const zs_cancel_req_t* cancelReq)
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
            rv = -1;
            break;
        }

        if (strcmp(cur_order->OrderSysID, cancelReq->OrderSysID) == 0 ||
            cur_order->OrderID == cancelReq->OrderID)
        {
            // found
            cur_order->Status = ZS_OS_Canceld;
            if (cur_order->FilledQty > 0)
            {
                cur_order->Status = ZS_OS_PartCancled;
                cur_order->CancelTime = (int32_t)(ztl_intdatetime() % 100000000);
            }

            // order rtn
            slippage->Handler(slippage, ZS_SDT_Order, cur_order, sizeof(*cur_order));

            // erase
            ztl_dlist_erase(order_list, iter);

            ztl_mp_free(slippage->MemPool, cur_order);

            rv = 0;
            break;
        }
    }
    ztl_dlist_iter_del(order_list, iter);
    return rv;
}


static void zs_slippage_process_neworder(zs_slippage_t* slippage)
{
    // 新订单先发送一个确认
    ztl_dlist_t* new_orders;
    ztl_dlist_iterator_t* iter;

    new_orders = slippage->NewOrders;
    if (ztl_dlist_size(new_orders) == 0) {
        return;
    }

    iter = ztl_dlist_iter_new(new_orders, ZTL_DLSTART_HEAD);
    while (true)
    {
        zs_order_t* cur_order;
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
    int     fill_qty;
    double  filled_price;

    ztl_dlist_t* order_list;
    ztl_dlist_iterator_t* iter;

    zs_slippage_process_neworder(slippage);

    if (ztl_dlist_size(slippage->CancelRequests) > 0)
    {
        iter = ztl_dlist_iter_new(slippage->CancelRequests, ZTL_DLSTART_HEAD);
        while (true)
        {
            zs_cancel_req_t* cancel_req;
            cancel_req = ztl_dlist_next(slippage->CancelRequests, iter);
            if (!cancel_req) {
                break;
            }

            if (zs_slippage_process_cancel(slippage, cancel_req) == 0)
            {
                ztl_mp_free(slippage->MemPool, cancel_req);
            }
        }
        ztl_dlist_iter_del(slippage->CancelRequests, iter);
    }

    if (ztl_dlist_size(slippage->OrderList) == 0)
    {
        return 0;
    }

    order_list = slippage->OrderList;
    iter = ztl_dlist_iter_new(order_list, ZTL_DLSTART_HEAD);
    while (true)
    {
        fill_qty = 0;
        filled_price = 0;

        zs_order_t* cur_order;
        cur_order = ztl_dlist_next(order_list, iter);
        if (!cur_order) {
            rv = 0;
            break;
        }

        // get the order fill price & volume
        if (bar_reader) {
            rv = slippage->SlippageModel->process_order_by_bar(
                slippage->SlippageModel, bar_reader, cur_order, 
                &filled_price, &fill_qty);
        }
        else if (tick)
        {
            rv = slippage->SlippageModel->process_order_by_tick(
                slippage->SlippageModel, tick, cur_order,
                &filled_price, &fill_qty);
        }
        else {
            rv = -1;
            break;
        }

        if (rv != 0)
        {
            continue;
        }

        // order rtn
        zs_update_order(cur_order, fill_qty);
        slippage->Handler(slippage, ZS_SDT_Order, cur_order, sizeof(*cur_order));

        // trade rtn
        zs_trade_t trade = { 0 };
        zs_generate_trade(&trade, cur_order, filled_price, fill_qty);
        slippage->Handler(slippage, ZS_SDT_Trade, &trade, sizeof(trade));

        if (cur_order->Status == ZS_OS_Filled)
        {
            ztl_dlist_erase(order_list, iter);

            ztl_mp_free(slippage->MemPool, cur_order);
        }
    }
    ztl_dlist_iter_del(order_list, iter);

    return rv;
}

int zs_slippage_process_bybar(zs_slippage_t* slippage,
    zs_bar_reader_t* bar_reader)
{
    return zs_slippage_process_order(slippage, bar_reader, NULL);
}

int zs_slippage_process_bytick(zs_slippage_t* slippage, zs_tick_t* tick)
{
    return zs_slippage_process_order(slippage, NULL, tick);
    return 0;
}



static void zs_generate_order(zs_order_t* order, const zs_order_req_t* order_req, int filled_volume)
{
    strcpy(order->Symbol, order_req->Symbol);
    strcpy(order->AccountID, order_req->AccountID);
    order->ExchangeID = order_req->ExchangeID;
    order->Sid = order_req->Sid;
    order->OrderPrice = order_req->OrderPrice;
    order->OrderQty = order_req->OrderQty;
    order->FilledQty = filled_volume;

    order->Direction = order_req->Direction;
    order->OffsetFlag = order_req->OffsetFlag;
    order->OrderType = order_req->OrderType;
    order->OrderID = order_req->OrderID;

    order->Status = ZS_OS_Accepted;

    if (filled_volume == 0 && order->FilledQty == 0) {
        order->Status = ZS_OS_Accepted;
    }
    else if ((filled_volume + order->FilledQty) < order_req->OrderQty) {
        order->Status = ZS_OS_PartFilled;
    }
    else if ((filled_volume + order->FilledQty) == order_req->OrderQty) {
        order->Status = ZS_OS_Filled;
    }

    if (!order->OrderSysID[0])
    {
        uint32_t sysid = ztl_atomic_add(&_zs_order_sys_id, 1);
        sprintf(order->OrderSysID, "%11u", sysid);

        int64_t dt = ztl_intdatetime();
        order->OrderTime = (int32_t)(dt % 100000000);
    }

    order->FrontID = 1;
    order->SessionID = 1;
}

static void zs_update_order(zs_order_t* order, int filled_qty)
{
    if (filled_qty == 0 && order->FilledQty == 0) {
        order->Status = ZS_OS_Accepted;
    }
    else if ((filled_qty + order->FilledQty) < order->OrderQty) {
        order->Status = ZS_OS_PartFilled;
        order->FilledQty += filled_qty;
    }
    else if ((filled_qty + order->FilledQty) == order->OrderQty) {
        order->Status = ZS_OS_Filled;
        order->FilledQty += filled_qty;
    }
}

static void zs_generate_trade(zs_trade_t* trade, const zs_order_t* order, double filled_price, int filled_qty)
{
    strcpy(trade->Symbol, order->Symbol);
    trade->ExchangeID = order->ExchangeID;
    strcpy(trade->AccountID, order->AccountID);

    uint32_t tid = ztl_atomic_add(&_zs_trade_id, 1);
    sprintf(trade->TradeID, "%u", tid);

    trade->OrderID = order->OrderID;
    trade->Price = filled_price;
    trade->Volume = filled_qty;

    trade->Direction = order->Direction;
    trade->OffsetFlag = order->OffsetFlag;

    int64_t dt = ztl_intdatetime();
    trade->TradeTime = (int32_t)(dt % 100000000);
}

/////////////////////////////////////////////////////////////////////////////////////

/* Checks whether the fill price is worse than the order's limit price */
static bool zs_fill_worse_than_limit(double filled_price, const zs_order_req_t* order_req)
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

static int _zs_process_order_volume_share(zs_slippage_model_t* slippage_model,
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
            return 0;
        }
    }
    else if (order->Direction == ZS_D_Short)
    {
        if (price <= order->OrderPrice)
        {
            *pfilled_price = price;
            *pfilled_qty = order->OrderQty;
            return 0;
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

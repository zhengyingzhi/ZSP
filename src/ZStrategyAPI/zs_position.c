#include <assert.h>

#include "zs_blotter.h"
#include "zs_constants_helper.h"
#include "zs_position.h"
#include "zs_order_list.h"

#include "zs_error.h"


static double calculate_margin(double price, int volume, int multiplier, double margin_ratio)
{
    return price * volume * multiplier * margin_ratio;
}


static int _zs_position_new_order(zs_position_engine_t* pos, ZSDirection direction, ZSOffsetFlag offset, int32_t order_qty)
{
    assert(offset != ZS_OF_Open);
    if (direction == ZS_D_Long)
    {
        if (order_qty > pos->ShortAvail && pos->ShortFrozen > pos->ShortPos) {
            return ZS_ERR_AvailClose;
        }

        if (offset == ZS_OF_Close || offset == ZS_OF_CloseYd)
        {
            if (order_qty > pos->ShortYdPos) {
                pos->ShortTdFrozen += order_qty - pos->ShortYdPos;
                pos->ShortYdFrozen += pos->ShortYdPos;
            }
            else {
                pos->ShortYdFrozen += order_qty;
            }
        }
        else if (offset == ZS_OF_CloseToday)
        {
            if (order_qty > (pos->ShortTdPos - pos->ShortTdFrozen) &&
                pos->ShortTdFrozen > pos->ShortTdPos) {
                return ZS_ERR_AvailClose;
            }
            pos->ShortTdFrozen += order_qty;
        }
        pos->ShortFrozen += order_qty;
        pos->ShortAvail -= order_qty;
    }
    else if (direction == ZS_D_Short)
    {
        if (order_qty > pos->LongAvail && pos->LongFrozen > pos->LongPos) {
            return ZS_ERR_AvailClose;
        }

        if (offset == ZS_OF_Close || offset == ZS_OF_CloseYd)
        {
            if (order_qty > pos->LongYdPos) {
                pos->LongTdFrozen += order_qty - pos->LongYdPos;
                pos->LongYdFrozen += pos->LongYdPos;
            }
            else {
                pos->ShortYdFrozen += order_qty;
            }
        }
        else if (offset == ZS_OF_CloseToday)
        {
            if (order_qty > (pos->LongTdPos - pos->LongTdFrozen) &&
                pos->LongTdFrozen > pos->LongTdPos) {
                return ZS_ERR_AvailClose;
            }
            pos->LongTdFrozen += order_qty;
        }
        pos->LongFrozen += order_qty;
        pos->LongAvail -= order_qty;
    }
    return ZS_OK;
}


zs_position_engine_t* zs_position_create(zs_blotter_t* blotter, ztl_pool_t* pool, zs_contract_t* contract)
{
    zs_position_engine_t* pos_engine;
    if (pool)
        pos_engine = (zs_position_engine_t*)ztl_pcalloc(pool, sizeof(zs_position_engine_t));
    else
        pos_engine = (zs_position_engine_t*)malloc(sizeof(zs_position_engine_t));
    pos_engine->Pool = pool;

    pos_engine->Blotter = blotter;
    pos_engine->Log     = blotter->Log;
    pos_engine->Contract= contract;
    if (contract)
    {
        pos_engine->Sid = contract->Sid;
        pos_engine->Multiplier = contract->Multiplier;
        pos_engine->PriceTick = contract->PriceTick;
        pos_engine->LongMarginRatio = contract->LongMarginRateByMoney;
        pos_engine->ShortMarginRatio = contract->ShortMarginRateByMoney;
    }

    pos_engine->PositionDetails = ztl_dlist_create(1024);


    pos_engine->handle_order_req = zs_position_handle_order_req;
    pos_engine->handle_order_rtn = zs_position_handle_order_rtn;
    pos_engine->handle_trade_rtn = zs_position_handle_trade_rtn;
    pos_engine->sync_last_price = zs_position_sync_last_price;
    pos_engine->handle_position_rsp = zs_position_handle_pos_rsp;
    pos_engine->handle_pos_detail_rsp = zs_position_handle_pos_detail_rsp;

    return pos_engine;
}

void zs_position_release(zs_position_engine_t* pos_engine)
{
    if (pos_engine->PositionDetails)
    {
        ztl_dlist_iterator_t* iter;
        zs_position_detail_t* pos_detail;

        iter = ztl_dlist_iter_new(pos_engine->PositionDetails, ZTL_DLSTART_HEAD);
        while (true)
        {
            pos_detail = ztl_dlist_next(pos_engine->PositionDetails, iter);
            if (!pos_detail) {
                break;
            }
            free(pos_detail);
        }
        ztl_dlist_iter_del(pos_engine->PositionDetails, iter);

        ztl_dlist_release(pos_engine->PositionDetails);
    }

    if (!pos_engine->Pool)
    {
        free(pos_engine);
    }
}

int zs_position_handle_order_req(zs_position_engine_t* pos_engine,
    ZSDirection direction, ZSOffsetFlag offset, int order_qty)
{
    if (offset == ZS_OF_Open) {
        return ZS_OK;
    }

    return _zs_position_new_order(pos_engine, direction, offset, order_qty);
}

int zs_position_handle_order_rtn(zs_position_engine_t* pos_engine, zs_order_t* order)
{
    if (order->OffsetFlag == ZS_OF_Open) {
        return ZS_OK;
    }

    // 仅处理撤单和拒单，解冻冻结数量
    if (!is_finished_status(order->OrderStatus) || order->OrderStatus == ZS_OS_Filled)
    {
        return ZS_ERR_OrderFinished;
    }

    // 剩余未成交数量
    int residual_qty = order->OrderQty - order->FilledQty;

    if (order->Direction == ZS_D_Long)
    {
        pos_engine->ShortFrozen -= residual_qty;
        pos_engine->ShortAvail += residual_qty;

        if (order->OffsetFlag == ZS_OF_Close || order->OffsetFlag == ZS_OF_CloseYd)
        {
            if (residual_qty > pos_engine->ShortYdFrozen) {
                pos_engine->ShortTdFrozen -= residual_qty - pos_engine->ShortYdFrozen;
                pos_engine->ShortYdFrozen = 0;
            }
            else {
                pos_engine->ShortYdFrozen -= residual_qty;
            }
        }
        else if (order->OffsetFlag == ZS_OF_CloseToday)
        {
            pos_engine->ShortTdFrozen -= residual_qty;
        }
    }
    else if (order->Direction == ZS_D_Short)
    {
        pos_engine->LongFrozen -= residual_qty;
        pos_engine->LongAvail += residual_qty;

        if (order->OffsetFlag == ZS_OF_Close || order->OffsetFlag == ZS_OF_CloseYd)
        {
            if (residual_qty > pos_engine->LongYdFrozen) {
                pos_engine->LongTdFrozen -= residual_qty - pos_engine->LongYdFrozen;
                pos_engine->LongYdFrozen = 0;
            }
            else {
                pos_engine->LongYdFrozen -= residual_qty;
            }
        }
        else if (order->OffsetFlag == ZS_OF_CloseToday)
        {
            pos_engine->LongTdFrozen -= residual_qty;
        }
    }
    return ZS_OK;
}

double zs_position_handle_trade_rtn(zs_position_engine_t* pos_engine, zs_trade_t* trade)
{
    double margin_ratio, margin, realized_pnl;
    double price, filled_money;
    int volume;
    ZSDirection direction;
    ZSOffsetFlag offset;

    margin = 0;
    realized_pnl = 0;
    price = trade->Price;
    volume = trade->Volume;
    direction = trade->Direction;
    offset = trade->OffsetFlag;
    filled_money = price * volume;      // no multiplier ?

    // 保证金
    margin_ratio = (direction == ZS_D_Long) ? pos_engine->LongMarginRatio : pos_engine->ShortMarginRatio;

    // 开仓保存一条明细
    if (trade->OffsetFlag == ZS_OF_Open)
    {
        // TODO: 
        // zs_position_detail_t pos_detail;
        // append to PositionDetails
    }

    if (direction == ZS_D_Long && offset == ZS_OF_Open)
    {
        // 买开仓
        pos_engine->LongPos += volume;
        pos_engine->LongTdPos += volume;
        pos_engine->LongMargin += margin;
        pos_engine->LongCost += filled_money;

        // A股T+1
        if (pos_engine->Contract->ProductClass != ZS_PC_Stock) {
            pos_engine->LongAvail += volume;
        }

        pos_engine->LongPrice = pos_engine->LongCost / pos_engine->LongPos;
        pos_engine->LongUpdateTime = trade->TradeTime;
    }
    else if (direction == ZS_D_Short && offset == ZS_OF_Open)
    {
        // 卖开仓
        pos_engine->ShortPos += volume;
        pos_engine->ShortTdPos += volume;
        pos_engine->ShortMargin += margin;
        pos_engine->ShortCost += filled_money;

        if (pos_engine->Contract->ProductClass != ZS_PC_Stock) {
            pos_engine->ShortAvail += volume;
        }

        pos_engine->ShortPrice = pos_engine->ShortCost / pos_engine->ShortPos;
        pos_engine->ShortUpdateTime = trade->TradeTime;
    }
    else if (direction == ZS_D_Long)
    {
        // 买平仓
        pos_engine->ShortPos    -= volume;
        pos_engine->ShortFrozen -= volume;
        pos_engine->ShortMargin -= margin;
        pos_engine->ShortCost   -= filled_money;

        if (offset == ZS_OF_Close || offset == ZS_OF_CloseYd)
        {
            if (volume > pos_engine->ShortYdPos)
            {
                pos_engine->ShortTdPos -= volume - pos_engine->ShortYdPos;
                pos_engine->ShortTdFrozen -= volume - pos_engine->ShortYdPos;
                pos_engine->ShortYdPos = 0;
                pos_engine->ShortYdFrozen = 0;
            }
            else
            {
                pos_engine->ShortYdPos -= volume;
                pos_engine->ShortYdFrozen -= volume;
            }
        }
        else if (offset == ZS_OF_CloseToday)
        {
            pos_engine->ShortYdPos -= volume;
            pos_engine->ShortTdFrozen -= volume;
        }

        // 持仓明细更新
        // ...
        pos_engine->ShortUpdateTime = trade->TradeTime;
    }
    else if (direction == ZS_D_Short)
    {
        // 卖平仓
        pos_engine->LongPos     -= volume;
        pos_engine->LongFrozen  -= volume;
        pos_engine->LongMargin  -= margin;
        pos_engine->LongCost    -= filled_money;

        if (offset == ZS_OF_Close || offset == ZS_OF_CloseYd)
        {
            if (volume > pos_engine->LongYdPos)
            {
                pos_engine->LongTdPos -= volume - pos_engine->LongYdPos;
                pos_engine->LongTdFrozen -= volume - pos_engine->LongYdPos;
                pos_engine->LongYdPos = 0;
                pos_engine->LongYdFrozen = 0;
            }
            else
            {
                pos_engine->LongYdPos -= volume;
                pos_engine->LongYdFrozen -= volume;
            }
        }
        else if (offset == ZS_OF_CloseToday)
        {
            pos_engine->LongYdPos -= volume;
            pos_engine->LongTdFrozen -= volume;
        }

        // 持仓明细更新
        // ...
        pos_engine->LongUpdateTime = trade->TradeTime;
    }
    return realized_pnl;
}

void zs_position_handle_pos_rsp(zs_position_engine_t* pos_engine, zs_position_t* pos)
{
    // 更新持仓数据
    if (pos->Direction == ZS_D_Long)
    {
        pos_engine->LongPos     = pos->Position;
        pos_engine->LongYdPos   = pos->YdPosition;
        pos_engine->LongTdPos   = pos->Position - pos->YdPosition;
        pos_engine->LongAvail   = pos->Available;
        pos_engine->LongFrozen  = pos->Frozen;
        pos_engine->LongPrice   = pos->PositionPrice;   // TODO
        pos_engine->LongPnl     = pos->PositionPnl;
        pos_engine->LongMargin  = pos->UseMargin;
        zs_log_info(pos_engine->Log, "pos_engine long  pos:%d,%d,%d,px:%.2lf,pnl:%.2lf,margin:%.1lf\n",
            pos->Position, pos->YdPosition, pos_engine->LongTdPos, pos_engine->LongPrice,
            pos_engine->LongPnl, pos_engine->LongMargin);
    }
    else if (pos->Direction == ZS_D_Short)
    {
        pos_engine->ShortPos    = pos->Position;
        pos_engine->ShortYdPos  = pos->YdPosition;
        pos_engine->ShortTdPos  = pos->Position - pos->YdPosition;
        pos_engine->ShortAvail  = pos->Available;
        pos_engine->ShortFrozen = pos->Frozen;
        pos_engine->ShortPrice  = pos->PositionPrice;   // TODO
        pos_engine->ShortPnl    = pos->PositionPnl;
        pos_engine->ShortMargin = pos->UseMargin;
        zs_log_info(pos_engine->Log, "pos_engine short pos:%d,%d,%d,px:%.2lf,pnl:%.2lf,margin:%.1lf\n",
            pos->Position, pos->YdPosition, pos_engine->ShortTdPos, pos_engine->ShortPrice,
            pos_engine->ShortPnl, pos_engine->ShortMargin);
    }
}

void zs_position_handle_pos_detail_rsp(zs_position_engine_t* pos_engine, zs_position_detail_t* pos_detail)
{
    zs_position_detail_t* pos_detail_dup;
    pos_detail_dup = (zs_position_detail_t*)malloc(sizeof(zs_position_detail_t));
    memcpy(pos_detail_dup, pos_detail, sizeof(zs_position_detail_t));
    ztl_dlist_insert_tail(pos_engine->PositionDetails, pos_detail);
}

void zs_position_pnl_calc(zs_position_engine_t* pos_engine)
{
    pos_engine->LongPnl = pos_engine->LongPos * (pos_engine->LastPrice - pos_engine->LongPrice) * pos_engine->Multiplier;
    pos_engine->ShortPnl = pos_engine->ShortPos * (pos_engine->LastPrice - pos_engine->ShortPrice) * pos_engine->Multiplier;
}

double zs_position_price_calc(zs_position_engine_t* pos_engine, ZSDirection direction)
{
    double  price;
    int32_t volume;

    price = 0;
    volume = 0;

    // 遍历持仓明

    if (volume > 0)
        price = price / volume;

    return price;
}

void zs_position_price_calc_fast(zs_position_engine_t* pos_engine, ZSDirection direction)
{
    if (direction == ZS_D_Long)
        pos_engine->LongPrice = pos_engine->LongCost / pos_engine->LongPos;
    else if (direction == ZS_D_Short)
        pos_engine->ShortPrice = pos_engine->ShortCost / pos_engine->ShortPos;
}

void zs_position_sync_last_price(zs_position_engine_t* pos_engine, double lastpx)
{
    pos_engine->LastPrice = lastpx;
    zs_position_pnl_calc(pos_engine);

    pos_engine->TickUpdatedN += 1;
}

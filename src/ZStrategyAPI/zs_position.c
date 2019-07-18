#include <assert.h>

#include "zs_position.h"

#include "zs_order_list.h"


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
            // ERRORID: 可平仓位不够
            return -1;
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
            if (order_qty > pos->ShortTdPos - pos->ShortTdFrozen && pos->ShortTdFrozen > pos->ShortTdPos) {
                // ERRORID: 可平仓位不够
                return -1;
            }
            pos->ShortTdFrozen += order_qty;
        }
        pos->ShortFrozen += order_qty;
        pos->ShortAvail -= order_qty;
    }
    else if (direction == ZS_D_Short)
    {
        if (order_qty > pos->LongAvail && pos->LongFrozen > pos->LongPos) {
            // ERRORID: 可平仓位不够
            return -1;
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
            if (order_qty > pos->LongTdPos - pos->LongTdFrozen && pos->LongTdFrozen > pos->LongTdPos) {
                // ERRORID: 可平仓位不够
                return -1;
            }
            pos->LongTdFrozen += order_qty;
        }
        pos->LongFrozen += order_qty;
        pos->LongAvail -= order_qty;
    }
    return 0;
}


zs_position_engine_t* zs_position_create(ztl_pool_t* pool, zs_contract_t* contract)
{
    zs_position_engine_t* pos;
    if (pool)
        pos = (zs_position_engine_t*)ztl_pcalloc(pool, sizeof(zs_position_engine_t));
    else
        pos = (zs_position_engine_t*)malloc(sizeof(zs_position_engine_t));
    pos->Pool = pool;
    pos->Contract = contract;
    if (contract)
    {
        pos->Sid = contract->Sid;
        pos->Multiplier = contract->Multiplier;
        pos->PriceTick = contract->PriceTick;
        pos->LongMarginRatio = contract->LongMarginRateByMoney;
        pos->ShortMarginRatio = contract->ShortMarginRateByMoney;
    }

    pos->PositionDetails = ztl_dlist_create(64);


    pos->handle_order_req = zs_position_handle_order_req;
    pos->handle_order_rtn = zs_position_handle_order_rtn;
    pos->handle_trade_rtn = zs_position_handle_trade_rtn;
    pos->sync_last_price = zs_position_sync_last_price;

    return pos;
}

void zs_position_release(zs_position_engine_t* pos)
{
    if (pos->PositionDetails) {
        // TODO
        ztl_dlist_release(pos->PositionDetails);
    }

    if (!pos->Pool) {
        free(pos);
    }
}

int zs_position_handle_order_req(zs_position_engine_t* pos, zs_order_req_t* order_req)
{
    if (order_req->OffsetFlag == ZS_OF_Open) {
        return 0;
    }

    return _zs_position_new_order(pos, order_req->Direction, order_req->OffsetFlag, order_req->OrderQty);
}

int zs_position_handle_order_rtn(zs_position_engine_t* pos, zs_order_t* order)
{
    if (order->OffsetFlag == ZS_OF_Open) {
        return 0;
    }

    // 仅处理撤单和拒单，解冻冻结数量
    if (order->Status != ZS_OS_Canceld && 
        order->Status != ZS_OS_PartCancled && 
        order->Status != ZS_OS_Rejected)
    {
        return 0;
    }

    int residual_qty = order->OrderQty - order->FilledQty;

    if (order->Direction != ZS_D_Long)
    {
        pos->ShortFrozen -= residual_qty;
        if (order->OffsetFlag == ZS_OF_Close || order->OffsetFlag == ZS_OF_CloseYd)
        {
            if (residual_qty > pos->ShortYdFrozen) {
                pos->ShortTdFrozen -= residual_qty - pos->ShortYdFrozen;
                pos->ShortYdFrozen = 0;
            }
            else {
                pos->ShortYdFrozen -= residual_qty;
            }
        }
        else if (order->OffsetFlag == ZS_OF_CloseToday)
        {
            pos->ShortTdFrozen -= residual_qty;
        }
    }
    else if (order->Direction == ZS_D_Short)
    {
        pos->LongFrozen -= residual_qty;
        if (order->OffsetFlag == ZS_OF_Close || order->OffsetFlag == ZS_OF_CloseYd)
        {
            if (residual_qty > pos->LongYdFrozen) {
                pos->LongTdFrozen -= residual_qty - pos->LongYdFrozen;
                pos->LongYdFrozen = 0;
            }
            else {
                pos->LongYdFrozen -= residual_qty;
            }
        }
        else if (order->OffsetFlag == ZS_OF_CloseToday)
        {
            pos->LongTdFrozen -= residual_qty;
        }
    }
    return 0;
}

double zs_position_handle_trade_rtn(zs_position_engine_t* pos, zs_trade_t* trade)
{
    double margin_ratio, margin, realized_pnl;
    double price;
    int volume;
    ZSDirection direction;
    ZSOffsetFlag offset;

    margin = 0;
    realized_pnl = 0;
    price = trade->Price;
    volume = trade->Volume;
    direction = trade->Direction;
    offset = trade->OffsetFlag;

    // 保证金
    margin_ratio = (direction == ZS_D_Long) ? pos->LongMarginRatio : pos->ShortMarginRatio;

    // 开仓保存一条明细
    if (trade->OffsetFlag == ZS_OF_Open)
    {
        // zs_position_detail_t pos_detail;
        // append to PositionDetails
    }

    if (direction == ZS_D_Long && offset == ZS_OF_Open)
    {
        pos->LongPos += volume;
        pos->LongTdPos += volume;
        pos->LongMargin += margin;
        pos->LongCost += price * volume;

        if (pos->Contract->ProductClass != ZS_PC_Stock) {
            pos->LongAvail += volume;
        }

        pos->LongPrice = pos->LongCost / pos->LongPos;
        pos->LongUpdateTime = trade->TradeTime;
    }
    else if (direction == ZS_D_Long && offset == ZS_OF_Open)
    {
        pos->ShortPos += volume;
        pos->ShortTdPos += volume;
        pos->ShortMargin += margin;
        pos->ShortCost += price * volume;

        if (pos->Contract->ProductClass != ZS_PC_Stock) {
            pos->ShortAvail += volume;
        }

        pos->ShortPrice = pos->ShortCost / pos->ShortPos;
        pos->ShortUpdateTime = trade->TradeTime;
    }
    else if (direction == ZS_D_Long)
    {
        pos->ShortPos -= volume;
        pos->ShortFrozen -= volume;

        if (offset == ZS_OF_Close || offset == ZS_OF_CloseYd)
        {
            if (volume > pos->ShortYdPos)
            {
                pos->ShortTdPos -= volume - pos->ShortYdPos;
                pos->ShortTdFrozen -= volume - pos->ShortYdPos;
                pos->ShortYdPos = 0;
                pos->ShortYdFrozen = 0;
            }
            else
            {
                pos->ShortYdPos -= volume;
                pos->ShortYdFrozen -= volume;
            }
        }
        else if (offset == ZS_OF_CloseToday)
        {
            pos->ShortYdPos -= volume;
            pos->ShortTdFrozen -= volume;
        }

        // 持仓明细更新
        // ...
        pos->ShortUpdateTime = trade->TradeTime;
    }
    else if (direction == ZS_D_Short)
    {
        pos->LongPos -= volume;
        pos->LongFrozen -= volume;

        if (offset == ZS_OF_Close || offset == ZS_OF_CloseYd)
        {
            if (volume > pos->LongYdPos)
            {
                pos->LongTdPos -= volume - pos->LongYdPos;
                pos->LongTdFrozen -= volume - pos->LongYdPos;
                pos->LongYdPos = 0;
                pos->LongYdFrozen = 0;
            }
            else
            {
                pos->LongYdPos -= volume;
                pos->LongYdFrozen -= volume;
            }
        }
        else if (offset == ZS_OF_CloseToday)
        {
            pos->LongYdPos -= volume;
            pos->LongTdFrozen -= volume;
        }

        // 持仓明细更新
        // ...
        pos->LongUpdateTime = trade->TradeTime;
    }
    return realized_pnl;
}

void zs_position_pnl_calc(zs_position_engine_t* pos)
{
    pos->LongPnl = pos->LongPos * (pos->LastPrice - pos->LongPrice) * pos->Multiplier;
    pos->ShortPnl = pos->ShortPos * (pos->LastPrice - pos->ShortPrice) * pos->Multiplier;
}

double zs_position_price_calc(zs_position_engine_t* pos, ZSDirection direction)
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

void zs_position_price_calc_fast(zs_position_engine_t* pos, ZSDirection direction)
{
    if (direction == ZS_D_Long)
        pos->LongPrice = pos->LongCost / pos->LongPos;
    else if (direction == ZS_D_Short)
        pos->ShortPrice = pos->ShortCost / pos->ShortPos;
}

void zs_position_sync_last_price(zs_position_engine_t* pos, double lastpx)
{
    pos->LastPrice = lastpx;
    zs_position_pnl_calc(pos);

    pos->TickUpdatedN += 1;
}

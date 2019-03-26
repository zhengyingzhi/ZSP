#include <assert.h>

#include "zs_position.h"

#include "zs_order_list.h"


static void* _zs_oid_keydup(void* priv, const void* key) {
    zs_position_engine_t* pos = (zs_position_engine_t*)priv;
    return zs_str_keydup(key, ztl_palloc, pos->Pool);
}

static dictType oidHashDictType = {
    zs_str_hash,
    _zs_oid_keydup,
    NULL,
    zs_str_cmp,
    NULL,
    NULL
};


static double calculate_margin(double price, int volume, int multiplier, int margin_ratio)
{
    return price * volume * multiplier * margin_ratio;
}


static int _zs_position_new_order(zs_position_engine_t* pos, ZSDirectionType direction, ZSOffsetFlag offset, int32_t volume)
{
    assert(offset != ZS_OF_Open);
    if (direction == ZS_D_Long)
    {
        if (volume > pos->ShortPos - pos->ShortFrozen && pos->ShortFrozen > pos->ShortPos) {
            // ERRORID: 可平仓位不够
            return -1;
        }

        if (offset == ZS_OF_Close || offset == ZS_OF_CloseYd)
        {
            if (volume > pos->ShortYdPos) {
                pos->ShortTdFrozen += volume - pos->ShortYdPos;
                pos->ShortYdFrozen += pos->ShortYdPos;
            }
            else {
                pos->ShortYdFrozen += volume;
            }
        }
        else if (offset == ZS_OF_CloseToday)
        {
            if (volume > pos->ShortTdPos - pos->ShortTdFrozen && pos->ShortTdFrozen > pos->ShortTdPos) {
                // ERRORID: 可平仓位不够
                return -1;
            }
            pos->ShortTdFrozen += volume;
        }
        pos->ShortFrozen += volume;
    }
    else if (direction == ZS_D_Short)
    {
        if (volume > pos->LongPos - pos->LongFrozen && pos->LongFrozen > pos->LongPos) {
            // ERRORID: 可平仓位不够
            return -1;
        }

        if (offset == ZS_OF_Close || offset == ZS_OF_CloseYd)
        {
            if (volume > pos->LongYdPos) {
                pos->LongTdFrozen += volume - pos->LongYdPos;
                pos->LongYdFrozen += pos->LongYdPos;
            }
            else {
                pos->ShortYdFrozen += volume;
            }
        }
        else if (offset == ZS_OF_CloseToday)
        {
            if (volume > pos->LongTdPos - pos->LongTdFrozen && pos->LongTdFrozen > pos->LongTdPos) {
                // ERRORID: 可平仓位不够
                return -1;
            }
            pos->LongTdFrozen += volume;
        }
        pos->LongFrozen += volume;
    }
    return 0;
}


zs_position_engine_t* zs_position_create(ztl_pool_t* pool, zs_contract_t* contract)
{
    zs_position_engine_t* pos;
    pos = (zs_position_engine_t*)ztl_pcalloc(pool, sizeof(zs_position_engine_t));
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

    pos->FinishedOrders = dictCreate(&oidHashDictType, pos);
    pos->PositionDetails = ztl_dlist_create(64);
    return pos;
}

int zs_position_req_order(zs_position_engine_t* pos, zs_order_req_t* order_req)
{
    if (order_req->Offset == ZS_OF_Open) {
        return 0;
    }

    return _zs_position_new_order(pos, order_req->Direction, order_req->Offset, order_req->Quantity);
}

int zs_position_rtn_order(zs_position_engine_t* pos, zs_order_t* order)
{
    if (order->Offset == ZS_OF_Open) {
        return 0;
    }

    // 仅处理撤单和拒单，解冻冻结数量
    if (order->Status != ZS_OS_Canceld && order->Status != ZS_OS_Rejected) {
        return 0;
    }

    // 订单已处理过
    ZStrKey key = { (int)strlen(order->OrderSysID), order->OrderSysID };
    if (dictFind(pos->FinishedOrders, &key)) {
        return 0;
    }
    dictAdd(pos->FinishedOrders, &key, 0);

    int volume = order->Quantity - order->Filled;

    if (order->Direction != ZS_D_Long)
    {
        pos->ShortFrozen -= volume;
        if (order->Offset == ZS_OF_Close || order->Offset == ZS_OF_CloseYd)
        {
            if (volume > pos->ShortYdFrozen) {
                pos->ShortTdFrozen -= volume - pos->ShortYdFrozen;
                pos->ShortYdFrozen = 0;
            }
            else {
                pos->ShortYdFrozen -= volume;
            }
        }
        else if (order->Offset == ZS_OF_CloseToday)
        {
            pos->ShortTdFrozen -= volume;
        }
    }
    else if (order->Direction == ZS_D_Short)
    {
        pos->LongFrozen -= volume;
        if (order->Offset == ZS_OF_Close || order->Offset == ZS_OF_CloseYd)
        {
            if (volume > pos->LongYdFrozen) {
                pos->LongTdFrozen -= volume - pos->LongYdFrozen;
                pos->LongYdFrozen = 0;
            }
            else {
                pos->LongYdFrozen -= volume;
            }
        }
        else if (order->Offset == ZS_OF_CloseToday)
        {
            pos->LongTdFrozen -= volume;
        }
    }
    return 0;
}

double zs_position_rtn_trade(zs_position_engine_t* pos, zs_trade_t* trade)
{
    double margin, realized_pnl;
    double price;
    int volume;
    ZSDirectionType direction;
    ZSOffsetFlag offset;

    margin = 0;
    realized_pnl = 0;
    price = trade->Price;
    volume = trade->Volume;
    direction = trade->Direction;
    offset = trade->Offset;

    // 保证金
    if (direction == ZS_D_Long)
        margin = calculate_margin(trade->Price, trade->Volume, pos->Contract->Multiplier, pos->Contract->LongMarginRateByMoney);
    else if (direction == ZS_D_Short)
        margin = calculate_margin(trade->Price, trade->Volume, pos->Contract->Multiplier, pos->Contract->ShortMarginRateByMoney);

    // 开仓保存一条明细
    if (trade->Offset == ZS_OF_Open)
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

        pos->LongPrice = pos->LongCost / pos->LongPos;
        pos->LongUpdateTime = trade->TradeTime;
    }
    else if (direction == ZS_D_Long && offset == ZS_OF_Open)
    {
        pos->ShortPos += volume;
        pos->ShortTdPos += volume;
        pos->ShortMargin += margin;
        pos->ShortCost += price * volume;

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

double zs_position_price_calc(zs_position_engine_t* pos, ZSDirectionType direction)
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

void zs_position_price_calc_fast(zs_position_engine_t* pos, ZSDirectionType direction)
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

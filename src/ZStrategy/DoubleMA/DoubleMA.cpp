#include "DoubleMA.h"


void* strategy_dma_create(zs_cta_strategy_t* context, const char* setting)
{
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)calloc(1, sizeof(strategy_dma_t));

    strcpy(instance->symbol, "rb1910");
    instance->exchangeid = ZS_EI_SHFE;

    // we could assign our user data to UserData field
    context->UserData = NULL;

    fprintf(stderr, "dma create object:%p\n", instance);

    return instance;
}

void strategy_dma_release(void* obj, zs_cta_strategy_t* context)
{
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    if (instance)
    {
        free(instance);
        context->UserData = NULL;
    }
}

int strategy_dma_is_trading_symbol(void* obj, zs_cta_strategy_t* context, zs_sid_t sid)
{
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    if (sid == instance->sid)
        return 1;
    return 0;
}

// 策略
void strategy_dma_on_init(void* obj, zs_cta_strategy_t* context)
{
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    // the sid will be used usually
    instance->sid = context->lookup_sid(context, instance->exchangeid, instance->symbol, 6);

    // get conf params
    context->get_conf_val(context, "period", instance->period, sizeof(instance->period), ZS_CT_String);
    context->get_conf_val(context, "volume", &instance->volume, sizeof(instance->volume), ZS_CT_Int32);
    context->get_conf_val(context, "slippage", &instance->slippage, sizeof(instance->slippage), ZS_CT_Int32);
    context->get_conf_val(context, "fast_window", &instance->fast_window, sizeof(instance->fast_window), ZS_CT_Int32);
    context->get_conf_val(context, "slow_window", &instance->slow_window, sizeof(instance->slow_window), ZS_CT_Int32);
    context->get_conf_val(context, "max_symbol_pos", &instance->max_symbol_pos, sizeof(instance->max_symbol_pos), ZS_CT_Double);

    fprintf(stderr, "dma init\n");
    context->write_log(context, "dma init symbol:%s, period:%s, vol:%d, fast:%d, slow:%d",
        instance->symbol, instance->period, instance->volume, instance->fast_window, instance->slow_window);
}

void strategy_dma_on_start(void* obj, zs_cta_strategy_t* context)
{
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    fprintf(stderr, "dma start\n");

    context->subscribe(context, instance->sid);
}

void strategy_dma_on_stop(void* obj, zs_cta_strategy_t* context)
{
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    fprintf(stderr, "dma stop\n");
}

void strategy_dma_on_update(void* obj, zs_cta_strategy_t* context, void* data, int size)
{
    // data is the setting (json type)

    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    fprintf(stderr, "dma update\n");
}

void strategy_dma_handle_order(void* obj, zs_cta_strategy_t* context, zs_order_t* order)
{
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    fprintf(stderr, "dma handle order: id:%s, symbol:%s,dir:%d,qty:%d,price:%.2f,offset:%d\n",
        order->OrderID, order->Symbol, order->Direction, order->OrderQty, order->OrderPrice, order->OffsetFlag);
}

void strategy_dma_handle_trade(void* obj, zs_cta_strategy_t* context, zs_trade_t* trade)
{
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    fprintf(stderr, "dma handle trade: id:%s, symbol:%s,dir:%d,qty:%d,price:%.2f,offset:%d\n",
        trade->OrderID, trade->Symbol, trade->Direction, trade->Volume, trade->Price, trade->OffsetFlag);
}

// 行情通知
void strategy_dma_handle_bar(void* obj, zs_cta_strategy_t* context, zs_bar_reader_t* bar_reader)
{
    zs_bar_t* bar;
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    bar = &bar_reader->Bar;

    fprintf(stderr, "dma handle_bar symbol:%s, openpx:%.2lf, closepx:%.2lf, vol:%lld\n",
        bar->Symbol, bar->OpenPrice, bar->ClosePrice, bar->Volume);
}

void strategy_dma_handle_tick(void* obj, zs_cta_strategy_t* context, zs_tick_t* tick)
{
    strategy_dma_t* instance;
    instance = (strategy_dma_t*)obj;

    // visit the tick data
    fprintf(stderr, "dma handle_tick symbol:%s, lastpx:%.2lf, vol:%lld\n",
        tick->Symbol, tick->LastPrice, tick->Volume);

    instance->index += 1;
    if (instance->index == 2)
    {
        // try send an order
        int rv = context->order(context, instance->sid, instance->volume, tick->LastPrice, ZS_D_Long, ZS_OF_Open);
        fprintf(stderr, "dma send order rv:%d\n", rv);
    }
}


//////////////////////////////////////////////////////////////////////////
static zs_strategy_entry_t strategy_dma =
{
    "strategy_dma",     // strategy name
    "yizhe",            // author
    "1.0.0",            // version
    0,                  // flag
    NULL,               // hlib

    strategy_dma_create,
    strategy_dma_release,
    strategy_dma_is_trading_symbol,

    strategy_dma_on_init,
    strategy_dma_on_start,
    strategy_dma_on_stop,
    strategy_dma_on_update,
    NULL,   // on timer
    NULL,   // before_trading_start
    strategy_dma_handle_order,
    strategy_dma_handle_trade,

    strategy_dma_handle_bar,
    strategy_dma_handle_tick,
    NULL
};


//////////////////////////////////////////////////////////////////////////
// 策略加载入口，需要导出，用于外部可获取策略的回调接口
DMA_API int DMA_API_STDCALL strategy_entry(zs_strategy_entry_t** ppentry)
{
    *ppentry = &strategy_dma;
    return 0;
}


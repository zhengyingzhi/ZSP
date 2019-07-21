#include <string.h>

#include "zs_algorithm.h"

#include "zs_algorithm_api.h"

#include "zs_assets.h"

#include "zs_core.h"

#include "zs_cta_strategy.h"

#include "zs_strategy_demo.h"

#include "zs_data_portal.h"

#include "zs_protocol.h"


/* 一个调试策略demo */
typedef struct my_strategy_demo_s
{
    int             index;
    zs_sid_t        sid;
    ZSExchangeID    exchangeid;
    char            accountID[16];
    char            symbol[32];
}my_strategy_demo_t;


void* stg_demo_create(zs_cta_strategy_t* context, const char* setting)
{
    my_strategy_demo_t* instance;
    instance = (my_strategy_demo_t*)calloc(1, sizeof(my_strategy_demo_t));

    strcpy(instance->symbol, "rb1910");
    instance->exchangeid = ZS_EI_SHFE;
    instance->sid = context->lookup_sid(context, instance->exchangeid, instance->symbol, 6);

    // we could assign our user data to UserData field
    context->UserData = NULL;

    return instance;
}

void stg_demo_release(my_strategy_demo_t* instance, zs_cta_strategy_t* context)
{
    if (instance)
    {
        free(instance);
        context->UserData = NULL;
    }
}

int stg_demo_is_trading_symbol(my_strategy_demo_t* instance, zs_cta_strategy_t* context, zs_sid_t sid)
{
    if (sid == instance->sid)
        return 1;
    return 0;
}

// 策略
void stg_demo_on_init(my_strategy_demo_t* instance, zs_cta_strategy_t* context)
{
    fprintf(stderr, "stg demo init\n");
}

void stg_demo_on_start(my_strategy_demo_t* instance, zs_cta_strategy_t* context)
{
    fprintf(stderr, "stg demo start\n");

    context->subscribe(context, instance->sid);
}

void stg_demo_on_stop(my_strategy_demo_t* instance, zs_cta_strategy_t* context)
{
    fprintf(stderr, "stg demo stop\n");
}

void stg_demo_on_update(my_strategy_demo_t* instance, zs_cta_strategy_t* context, void* data, int size)
{
    fprintf(stderr, "stg demo update\n");
}

void stg_demo_handle_order(my_strategy_demo_t* instance, zs_cta_strategy_t* context, zs_order_t* order)
{
    fprintf(stderr, "stg demo handle order: id:%s, symbol:%s,dir:%d,qty:%d,price:%.2f,offset:%d\n", 
        order->OrderID, order->Symbol, order->Direction, order->OrderQty, order->OrderPrice, order->OffsetFlag);
}

void stg_demo_handle_trade(my_strategy_demo_t* instance, zs_cta_strategy_t* context, zs_trade_t* trade)
{
    fprintf(stderr, "stg demo handle trade: id:%s, symbol:%s,dir:%d,qty:%d,price:%.2f,offset:%d\n",
        trade->OrderID, trade->Symbol, trade->Direction, trade->Volume, trade->Price, trade->OffsetFlag);
}

// 行情通知
void stg_demo_handle_bar(my_strategy_demo_t* instance, zs_cta_strategy_t* context, zs_bar_reader_t* bar_reader)
{
}

void stg_demo_handle_tick(my_strategy_demo_t* instance, zs_cta_strategy_t* context, zs_tick_t* tick)
{
    // visit the tick data
    fprintf(stderr, "demo handle_tick symbol:%s, lastpx:%.2lf, vol:%lld\n",
        tick->Symbol, tick->LastPrice, tick->Volume);

    instance->index += 1;
    if (instance->index == 2)
    {
        // try send an order
        int rv = context->order(context, instance->sid, 10, tick->LastPrice, ZS_D_Long, ZS_OF_Open);
        fprintf(stderr, "std demo send order rv:%d\n", rv);
    }
}


//////////////////////////////////////////////////////////////////////////
static zs_strategy_entry_t stg_demo =
{
    "strategy_demo",    // strategy name
    "zsp",              // author
    "1.0.0",            // version
    0,                  // flag
    NULL,               // hlib

    stg_demo_create,
    stg_demo_release,
    stg_demo_is_trading_symbol,

    stg_demo_on_init,
    stg_demo_on_start,
    stg_demo_on_stop,
    stg_demo_on_update,
    NULL,   // on timer
    NULL,   // before_trading_start
    stg_demo_handle_order,
    stg_demo_handle_trade,

    stg_demo_handle_bar,
    stg_demo_handle_tick,
    NULL
};

// 策略加载入口，需要导出，用于外部可获取策略的回调接口
int zs_demo_strategy_entry(zs_strategy_entry_t** ppentry)
{
    *ppentry = &stg_demo;
    return 0;
}



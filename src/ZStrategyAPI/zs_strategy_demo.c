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
    int index;
    char accountID[16];
    const char* symbols[4];
    // zs_cta_strategy_t* context;
}my_strategy_demo_t;


void* stg_demo_create(const char* setting)
{
    my_strategy_demo_t* stgdemo;
    stgdemo = (my_strategy_demo_t*)calloc(1, sizeof(my_strategy_demo_t));

    stgdemo->symbols[0] = "000001.SZSE";

    return stgdemo;
}

void stg_demo_release(zs_cta_strategy_t* context)
{
    if (context->Instance)
    {
        free(context->Instance);
        context->Instance = NULL;
    }
}

int stg_demo_is_trading_symbol(zs_cta_strategy_t* context, zs_sid_t sid)
{
    return 1;
}

// 策略
void stg_demo_on_init(void* instance)
{
    printf("stg demo init\n");
}

void stg_demo_on_start(void* instance)
{
    printf("stg demo start\n");
}

void stg_demo_on_stop(void* instance)
{
    printf("stg demo stop\n");
}

void stg_demo_on_update(zs_cta_strategy_t* context, void* data, int size)
{
    printf("stg demo update\n");
}

void stg_demo_handle_order(zs_cta_strategy_t* context, zs_order_t* order)
{
    printf("stg demo got order: %s,%d,%d,%.2f\n", order->Symbol, order->Direction, 
        order->Quantity, order->Price);
    // zs_cta_order(context, NULL);
}


void stg_demo_handle_trade(zs_cta_strategy_t* context, zs_trade_t* trade)
{
    printf("stg demo got trade: %s,%d,%d,%.2f\n", trade->Symbol, trade->Direction,
        trade->Volume, trade->Price);
}

// 行情通知
void stg_demo_handle_bar(zs_cta_strategy_t* context, zs_bar_reader_t* bar_reader)
{
    my_strategy_demo_t* mystg;
    //zs_portfolio_t* portfolio;
    double closepx;


    // mystg = (my_strategy_demo_t*)instance;
    mystg->index += 1;

    int exchangeid = 0;
    zs_sid_t sid = 10;
    const char* symbol = mystg->symbols[0];
    if (strstr(symbol, "SSE"))
        exchangeid = ZS_EI_SSE;
    else
        exchangeid = ZS_EI_SZSE;
    sid = zs_asset_lookup(context->AssetFinder, exchangeid, symbol, (int)strlen(symbol));

    closepx = bar_reader->current(bar_reader, sid, "close");
    printf("stg demo %d, closepx:%.2lf\n", mystg->index, closepx);

    // try send an order
    int rv = context->order(context, sid, 100, closepx, ZS_D_Long, ZS_OF_Open);
    printf("std demo send order rv:%d\n", rv);
}

void stg_demo_handle_tick(zs_cta_strategy_t* context, zs_tick_t* tick)
{
    // visit the tick data
}


//////////////////////////////////////////////////////////////////////////
static zs_strategy_entry_t stg_demo =
{
    "stg_demo",
#if 0
    "zsp",
    "1.0.0",
    0,
    NULL,
#else
    0,
#endif

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



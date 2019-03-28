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
    zs_cta_strategy_t* context;
}my_strategy_demo_t;

void* stg_demo_create(zs_cta_strategy_t* context, const char* reserve)
{
    my_strategy_demo_t* stgdemo;
    stgdemo = (my_strategy_demo_t*)calloc(1, sizeof(my_strategy_demo_t));

    stgdemo->symbols[0] = "000001.SZA";
    stgdemo->context = context;

    return stgdemo;
}

void stg_demo_release(void* stg)
{
    if (stg)
    {
        free(stg);
    }
}

// 策略
void stg_demo_on_init(void* stg)
{
    printf("stg demo init\n");
}

void stg_demo_on_start(void* stg)
{
    printf("stg demo start\n");
}

void stg_demo_on_stop(void* stg)
{
    printf("stg demo stop\n");
}

void stg_demo_on_order(void* stg, zs_cta_strategy_t* context, zs_order_t* order)
{
    printf("stg demo got order: %s,%d,%d,%.2f\n", order->Symbol, order->Direction, 
        order->Quantity, order->Price);
    // zs_cta_order(context, NULL);
}

void stg_demo_on_trade(void* stg, zs_cta_strategy_t* context, zs_trade_t* trade)
{
    printf("stg demo got trade: %s,%d,%d,%.2f\n", trade->Symbol, trade->Direction,
        trade->Volume, trade->Price);
}

// 行情通知
void stg_demo_on_bardata(void* stg, zs_cta_strategy_t* context, zs_bar_reader_t* barReader)
{
    my_strategy_demo_t* mystg;
    //zs_portfolio_t* portfolio;
    float closepx;

    mystg = (my_strategy_demo_t*)stg;
    mystg->index += 1;

    zs_sid_t sid = 10;
    const char* symbol = mystg->symbols[0];
    sid = zs_asset_lookup(context->AssetFinder, symbol, (int)strlen(symbol));

    closepx = barReader->current(barReader, sid, "close");
    printf("stg demo %d, closepx:%.2lf\n", mystg->index, closepx);

    // try send an order
    int rv = zs_order(context, NULL, sid, 100, closepx, 0, NULL);
    printf("std demo send order rv:%d\n", rv);
}

void stg_demo_on_tickdata(void* stg, zs_cta_strategy_t* context, zs_tick_t* tickData)
{
    // visit the tick data
}


//////////////////////////////////////////////////////////////////////////
static zs_strategy_api_t stg_demo =
{
    "stg_demo",
    NULL,
    NULL,
    0,
    0,
    stg_demo_create,
    stg_demo_release,
    stg_demo_on_init,
    stg_demo_on_start,
    stg_demo_on_stop,
    NULL,   // on update
    NULL,   // before_trading_start
    stg_demo_on_order,
    stg_demo_on_trade,

    stg_demo_on_bardata,
    stg_demo_on_tickdata,
    NULL
};

// 策略加载入口，需要导出，用于外部可获取策略的回调接口
int zs_demo_strategy_entry(zs_strategy_api_t** ppStragegy)
{
    *ppStragegy = &stg_demo;
    return 0;
}



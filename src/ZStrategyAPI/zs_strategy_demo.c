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
    char            account_id[16];     // 交易账号
    char            symbol[32];         // 交易合约
    int32_t         volume;             // 下单数量
    double          last_price;
}my_strategy_demo_t;


void* stg_demo_create(zs_cta_strategy_t* context, const char* setting)
{
    my_strategy_demo_t* instance;
    instance = (my_strategy_demo_t*)calloc(1, sizeof(my_strategy_demo_t));

    // some default values
    instance->volume = 1;
    instance->last_price = 0.0;

    // parse the setting
    zs_json_t* zjson;
    zjson = zs_json_parse(setting, (int)strlen(setting));
    if (!zjson) {
        return NULL;
    }

    strcpy(instance->account_id, context->pAccountID);

    zs_json_get_int(zjson, "Volume", &instance->volume);
    zs_json_get_string(zjson, "Symbol", instance->symbol, sizeof(instance->symbol));
    instance->exchangeid = ZS_EI_SHFE;

    zs_json_release(zjson);

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

    instance->sid = context->lookup_sid(context, instance->exchangeid, instance->symbol, 6);
    if (instance->sid == ZS_SID_INVALID) {
        // ERRORID: unknown symbol
        fprintf(stderr, "stg demo start but unknown symbol\n");
        return;
    }

    context->subscribe(context, instance->sid);
}

void stg_demo_on_stop(my_strategy_demo_t* instance, zs_cta_strategy_t* context)
{
    fprintf(stderr, "stg demo stop\n");
}

void stg_demo_on_update(my_strategy_demo_t* instance, zs_cta_strategy_t* context, void* data, int size)
{
    fprintf(stderr, "stg demo update\n");

    zs_json_t* zjson;
    zjson = zs_json_parse((char*)data, size);
    if (!zjson) {
        fprintf(stderr, "stg demo parse buffer failed\n");
        return;
    }

    ZSDirection direction;
    ZSOffsetFlag offset;

    int flag = -1;
    zs_json_get_int(zjson, "flag", &flag);
    if (flag == 0)
    {
        zs_account_t* account = NULL;
        context->get_trading_account(context, &account);
        if (account)
        {
            zs_fund_account_t* fa = &account->FundAccount;
            fprintf(stderr, "stg demo: bal:%.2lf, avail:%.2lf, frozen:%.2lf, margin:%.2lf\n",
                fa->Balance, fa->Available, fa->FrozenCash, fa->Margin);
        }

        zs_position_engine_t* pos_engine = NULL;
        context->get_account_position(context, &pos_engine, instance->sid);
        if (pos_engine)
        {
            fprintf(stderr, "stg demo: long:%d,short:%d,lastpx:%.lf\n",
                pos_engine->LongPos, pos_engine->ShortPos, pos_engine->LastPrice);
        }

        zs_order_t* orders[32] = { 0 };
        context->get_open_orders(context, orders, 32, ZS_SID_WILDCARD);
    }
    else if (flag == 1)
    {
        fprintf(stderr, "stg demo buy-open\n");
        if (instance->last_price < 0.1)
            return;

        direction = ZS_D_Long;
        offset = ZS_OF_Open;
    }
    else if (flag == 2)
    {
        fprintf(stderr, "stg demo sell-close\n");
        if (instance->last_price < 0.1)
            return;

        direction = ZS_D_Short;
        offset = ZS_OF_CloseToday;
    }
    else if (flag == 3)
    {
        fprintf(stderr, "stg demo sell-open\n");
        if (instance->last_price < 0.1)
            return;

        direction = ZS_D_Short;
        offset = ZS_OF_Open;
    }
    else if (flag == 4)
    {
        fprintf(stderr, "stg demo buy-close\n");
        if (instance->last_price < 0.1)
            return;

        direction = ZS_D_Long;
        offset = ZS_OF_CloseToday;
    }
    else {
        return;
    }

    int rv;
    rv = context->order(context, instance->sid, 1, instance->last_price,
        direction, offset);
    fprintf(stderr, "stg demo send order rv:%d\n", rv);

    zs_json_release(zjson);
}

void stg_demo_handle_order(my_strategy_demo_t* instance, zs_cta_strategy_t* context, zs_order_t* order)
{
    fprintf(stderr, "stg demo handle order: id:%s, symbol:%s,dir:%d,qty:%d,price:%.2f,offset:%d,status:%d\n", 
        order->OrderID, order->Symbol, order->Direction, order->OrderQty, order->OrderPrice, order->OffsetFlag, order->OrderStatus);
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
    static int count2 = 0;
    count2 += 1;
    if ((count2 & 7) == 0)
        fprintf(stderr, "demo handle_tick symbol:%s, lastpx:%.2lf, vol:%lld\n",
            tick->Symbol, tick->LastPrice, tick->Volume);

    instance->last_price = tick->LastPrice;

    instance->index += 1;
#if 0
    if (instance->index == 2)
    {
        // try send an order
        int rv = context->order(context, instance->sid, 10, tick->LastPrice, ZS_D_Long, ZS_OF_Open);
        fprintf(stderr, "std demo send order rv:%d\n", rv);
    }
#endif//0
}


//////////////////////////////////////////////////////////////////////////
static zs_strategy_entry_t stg_demo =
{
    "strategy_demo",    // strategy name
    "zsp",              // author
    "1.0.0",            // version
    0,                  // flag
    NULL,               // hlib
    ZS_ST_AssetSingle,  // 单标的策略

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



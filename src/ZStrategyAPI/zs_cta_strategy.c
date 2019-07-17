
#include <ZToolLib/ztl_dyso.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_data_portal.h"

#include "zs_event_engine.h"

#include "zs_cta_strategy.h"

#include "zs_strategy_engine.h"



int zs_cta_order(zs_cta_strategy_t* strategy, zs_sid_t sid, int order_qty, double order_price, ZSDirection direction, ZSOffsetFlag offset);
int zs_cta_place_order(zs_cta_strategy_t* strategy, zs_order_req_t* order_req);
int zs_cta_cancel(zs_cta_strategy_t* strategy, zs_cancel_req_t* cancel_req);
int zs_cta_cancel_all(zs_cta_strategy_t* strategy);


zs_cta_strategy_t* zs_cta_strategy_create(zs_strategy_engine_t* engine)
{
    zs_cta_strategy_t* strategy;
    strategy = malloc(sizeof(zs_cta_strategy_t));
    strategy->order = zs_cta_order;
    strategy->cancel_order = zs_cta_cancel;
    return strategy;
}

void zs_cta_strategy_release(zs_cta_strategy_t* strategy)
{
    if (strategy)
    {
        free(strategy);
    }
}

int zs_cta_order(zs_cta_strategy_t* strategy, zs_sid_t sid, int order_qty, double order_price, ZSDirection direction, ZSOffsetFlag offset)
{
    zs_order_req_t order_req;
    memset(&order_req, 0, sizeof(order_req));
    strncpy(order_req.AccountID, strategy->pAccountID, sizeof(order_req.AccountID) - 1);
    strncpy(order_req.UserID, strategy->pCustomID, sizeof(order_req.UserID) - 1);
    // fill symbol, exchangeid, brokerid ...
    order_req.Sid = sid;
    order_req.Quantity = order_qty;
    order_req.Price = order_price;
    order_req.Direction = direction;
    order_req.Offset = offset;

    return zs_cta_place_order(strategy, &order_req);
}

int zs_cta_place_order(zs_cta_strategy_t* strategy, zs_order_req_t* order_req)
{
    int rv;
    rv = zs_blotter_order(strategy->Blotter, order_req);
    if (rv == 0)
    {
        zs_strategy_engine_save_order(strategy->Engine, strategy, order_req);
    }
    return rv;
}

int zs_cta_cancel(zs_cta_strategy_t* strategy, zs_cancel_req_t* cancel_req)
{
    return -2;
}

int zs_cta_cancel_all(zs_cta_strategy_t* strategy)
{
    return -2;
}


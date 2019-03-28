
#include <ZToolLib/ztl_dyso.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_data_portal.h"

#include "zs_event_engine.h"

#include "zs_cta_strategy.h"

zs_cta_strategy_t* zs_cta_strategy_create(zs_strategy_engine_t* engine)
{
    zs_cta_strategy_t* strategy;
    strategy = NULL;
    return strategy;
}

void zs_cta_strategy_release(zs_cta_strategy_t* strategy)
{}

int zs_cta_order(zs_cta_strategy_t* strategy, zs_order_req_t* order_req)
{
    return -1;
}

int zs_cta_cancel(zs_cta_strategy_t* strategy, zs_cancel_req_t* cancel_req)
{
    return -2;
}



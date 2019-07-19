#include <assert.h>
#include <ctype.h>

#include <ZToolLib/ztl_utils.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_commission.h"

#include "zs_commission.h"


typedef union 
{
    char c[4];
    uint32_t i;
}ZSVarietyInt;

static uint32_t zs_get_variety_int(const char* symbol);

double zs_per_share_calculate(zs_commission_model_t* comm_model,
    const zs_order_t* order, const zs_trade_t* trade);
double zs_per_contract_calculate(zs_commission_model_t* comm_model,
    const zs_order_t* order, const zs_trade_t* trade);

static zs_commission_model_t zs_per_share_model = {
    "per_share",
    NULL,
    zs_per_share_calculate
};

static zs_commission_model_t zs_per_contract_model = {
    "per_contract",
    NULL,
    zs_per_contract_calculate
};


zs_commission_t* zs_commission_create(zs_algorithm_t* algo)
{
    zs_commission_t* comm;
    comm = (zs_commission_t*)ztl_pcalloc(algo->Pool, sizeof(zs_commission_t));

    comm->Algorithm = algo;
    comm->EquityModel = &zs_per_share_model;
    comm->EquityModel->UserData = comm;
    comm->FutureModel = &zs_per_contract_model;
    comm->FutureModel->UserData = comm;

    comm->VarietyMap = ztl_map_create(64);

    return comm;
}

void zs_commission_release(zs_commission_t* comm)
{
    if (comm)
    {
        if (comm->VarietyMap)
        {
            ztl_map_release(comm->VarietyMap);
            comm->VarietyMap = NULL;
        }
    }
}

void zs_commssion_set_per_share(zs_commission_t* comm, zs_comm_per_share_t* per_share_cost)
{
    comm->PerShareCost = *per_share_cost;
}

void zs_commssion_set_per_contract(zs_commission_t* comm, 
    zs_comm_per_contract_t* per_contract_cost, const char* symbol)
{
    if (!symbol || !symbol[0]) {
        return;
    }

    uint32_t vi;
    vi = zs_get_variety_int(symbol);

    zs_comm_per_contract_t* per_contract;
    per_contract = (zs_comm_per_contract_t*)ztl_pcalloc(comm->Algorithm->Pool,
        sizeof(zs_comm_per_contract_t));
    *per_contract = *per_contract_cost;

    ztl_map_add(comm->VarietyMap, vi, per_contract);
}

zs_commission_model_t* zs_commission_model_get(zs_commission_t* comm, int is_equity)
{
    if (is_equity) {
        return comm->EquityModel;
    }
    else {
        return comm->FutureModel;
    }
}

double zs_commission_calculate(zs_commission_t* comm, int is_equity,
    zs_order_t* order, zs_trade_t* trade)
{
    double commission;
    zs_commission_model_t* model;
    model = zs_commission_model_get(comm, is_equity);

    commission = model->calculate(model, order, trade);
    return commission;
}


static uint32_t zs_get_variety_int(const char* symbol)
{
#ifdef _DEBUG
    assert(sizeof(ZSVarietyInt) == 4);
#endif

    ZSVarietyInt vi;
    vi.i = 0;

    vi.c[0] = symbol[0];
    if (isalpha(symbol[1]))
        vi.c[1] = symbol[1];

    return vi.i;
}

double zs_per_share_calculate(zs_commission_model_t* comm_model,
    const zs_order_t* order, const zs_trade_t* trade)
{
    zs_commission_t* comm;
    zs_comm_per_share_t* per_share;

    double commission;
    double filled_money;

    comm = (zs_commission_t*)comm_model->UserData;
    per_share = &comm->PerShareCost;

    if (trade)
    {
        if (trade->Direction == ZS_D_Long) {
            commission = trade->Price * trade->Volume * per_share->BuyCost;
        }
        else {
            filled_money = trade->Price * trade->Volume;
            commission = filled_money * per_share->SellCost + filled_money * 0.001;
        }
    }
    else if (order)
    {
        if (order->Direction == ZS_D_Long) {
            commission = order->OrderPrice * order->OrderQty * per_share->BuyCost;
        }
        else {
            filled_money = order->OrderPrice * order->OrderQty;
            commission = filled_money * per_share->SellCost + filled_money * 0.001;
        }
    }
    else {
        commission = per_share->MinCost;
    }

    commission = ztl_max(commission, per_share->MinCost);

    return commission;
}

double zs_per_contract_calculate(zs_commission_model_t* comm_model,
    const zs_order_t* order, const zs_trade_t* trade)
{
    zs_commission_t* comm;
    const char* symbol;
    double price;
    int volume;
    int multiplier;
    uint32_t vi;
    ZSOffsetFlag offset;
    zs_sid_t sid;

    zs_comm_per_contract_t* per_contract;

    comm = (zs_commission_t*)comm_model->UserData;
    if (trade)
    {
        symbol = trade->Symbol;
        price = trade->Price;
        volume = trade->Volume;
        offset = trade->OffsetFlag;
        sid = trade->Sid;
    }
    else if (order)
    {
        symbol = order->Symbol;
        price = order->OrderPrice;
        volume = order->OrderQty;
        offset = order->OffsetFlag;
        sid = order->Sid;
    }
    else {
        return 0.0;
    }

    double commission;
    vi = zs_get_variety_int(symbol);
    per_contract = ztl_map_find(comm->VarietyMap, vi);
    if (!per_contract)
    {
        return 0.0;
    }

    if (per_contract->OpenCost > 0 && per_contract->OpenCost < 0.001)
    {
        // by volume
        if (offset == ZS_OF_Open)
            commission = volume * per_contract->OpenCost;
        else if (offset = ZS_OF_Close)
            commission = volume * per_contract->CloseCost;
        else
            commission = volume * per_contract->ClostTodayCost;
    }
    else
    {
        // by money
        multiplier = per_contract->Multiplier;
        if (multiplier == 0)
        {
            zs_contract_t* contract;
            contract = zs_asset_find_by_sid(comm->Algorithm->AssetFinder, sid);
            multiplier = contract->Multiplier;
            per_contract->Multiplier = multiplier;
        }

        if (offset == ZS_OF_Open)
            commission = price * volume * per_contract->OpenCost * multiplier;
        else if (offset = ZS_OF_Close)
            commission = price * volume * per_contract->CloseCost * multiplier;
        else
            commission = price * volume * per_contract->ClostTodayCost * multiplier;
    }

    return commission;
}


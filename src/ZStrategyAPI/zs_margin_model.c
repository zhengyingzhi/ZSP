#include <assert.h>
#include <ctype.h>

#include <ZToolLib/ztl_utils.h>

#include "zs_algorithm.h"
#include "zs_assets.h"
#include "zs_margin_model.h"


/// 期货保证金
static double calc_margin_future(zs_margin_model_t* model, zs_contract_t* contract,
    ZSDirection direction, int32_t volume, double price, double var);
/// ETF期权保证金
static double calc_margin_option_etf(zs_margin_model_t* model, zs_contract_t* contract,
    ZSDirection direction, int32_t volume, double price, double var);
/// 指数期权期权保证金
static double calc_margin_option_index(zs_margin_model_t* model, zs_contract_t* contract,
    ZSDirection direction, int32_t volume, double price, double var);
/// 商品期货期权保证金
static double calc_margin_option_commodity(zs_margin_model_t* model, zs_contract_t* contract,
    ZSDirection direction, int32_t volume, double price, double var);

int zs_margin_model_init(zs_margin_model_t* margin_model, ZSMarginAssetType asset_type)
{
    margin_model->AssetType = asset_type;

    switch (asset_type)
    {
    case ZS_MC_Equity:
    case ZS_MC_Future:
        margin_model->calculate = calc_margin_future;
        break;
    case ZS_MC_OptionETF:
        margin_model->calculate = calc_margin_option_etf;
        break;
    case ZS_MC_OptionCommodity:
        margin_model->calculate = calc_margin_option_commodity;
        break;
    case ZS_MC_OptionIndex:
        margin_model->calculate = calc_margin_option_index;
        break;
    default:
        return ZS_ERROR;
    }

    return ZS_OK;
}


static double calc_margin_future(zs_margin_model_t* model, zs_contract_t* contract,
    ZSDirection direction, int32_t volume, double price, double var)
{
    (void)var;
    if (direction == ZS_D_Long) {
        return volume * price * contract->Multiplier * contract->LongMarginRateByMoney;
    }
    else {
        return volume * price * contract->Multiplier * contract->ShortMarginRateByMoney;
    }
}

static double calc_margin_option_etf(zs_margin_model_t* model, zs_contract_t* contract,
    ZSDirection direction, int32_t volume, double price, double var)
{
    double delta, margin;
    double underlying_price;

    underlying_price = var;

    if (contract->OptionType == ZS_OPT_Call)
    {
        delta = ztl_max(contract->StrikePrice - underlying_price, 0);
        margin = (price + ztl_max(0.12 * underlying_price - delta, 0.07 * underlying_price)) \
            * volume * contract->Multiplier;
    }
    else
    {
        delta = ztl_max(underlying_price - contract->StrikePrice, 0);
        margin = ztl_min(price + max(0.12 * underlying_price - delta, 0.07 * contract->StrikePrice), contract->StrikePrice) \
            * volume * contract->Multiplier;
    }

    return margin;
}

static double calc_margin_option_index(zs_margin_model_t* model, zs_contract_t* contract,
    ZSDirection direction, int32_t volume, double price, double var)
{
    double delta, margin;
    double underlying_price;

    underlying_price = var;

    if (contract->OptionType == ZS_OPT_Call)
    {
        delta = ztl_max(contract->StrikePrice - underlying_price, 0);
        margin = (price + ztl_max(0.12 * underlying_price - delta, 0.07 * underlying_price)) \
            * volume * model->UnderlyingContract->Multiplier;
    }
    else
    {
        delta = ztl_max(underlying_price - contract->StrikePrice, 0);
        margin = ztl_min(price + ztl_max(0.12 * underlying_price - delta, 0.07 * contract->StrikePrice), contract->StrikePrice) \
            * volume * model->UnderlyingContract->Multiplier;
    }

    return margin;
}

static double calc_margin_option_commodity(zs_margin_model_t* model, zs_contract_t* contract,
    ZSDirection direction, int32_t volume, double price, double var)
{
    double delta, margin, underlying_margin;
    double underlying_price;

    underlying_price = var;

    // 标的保证金
    underlying_margin = calc_margin_future(model, model->UnderlyingContract, direction,
        volume, price, var);

    if (contract->OptionType == ZS_OPT_Call)
    {
        delta = (ztl_max(contract->StrikePrice - underlying_price, 0)) * contract->Multiplier;
        margin = (price * volume * contract->Multiplier + ztl_max(underlying_margin - 0.5 * delta, 0.5 * underlying_margin));
    }
    else
    {
        delta = (ztl_max(underlying_price - contract->StrikePrice, 0)) * contract->Multiplier;
        margin = price * volume * contract->Multiplier + ztl_max(underlying_margin - 0.5 * delta, 0.5 * underlying_margin);
    }

    return margin;
}

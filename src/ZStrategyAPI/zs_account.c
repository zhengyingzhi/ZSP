#include <ZToolLib/ztl_dlist.h>
#include <ZToolLib/ztl_utils.h>
#include <ZToolLib/ztl_palloc.h>

#include "zs_account.h"



static double calculate_margin(double price, int volume, int multiplier, double margin_ratio)
{
    return price * volume * multiplier * margin_ratio;
}

static void update_long_frozen_margin(zs_account_t* account, double frozen_margin)
{
    account->FrozenLongMargin += frozen_margin;
}

static void update_short_frozen_margin(zs_account_t* account, double frozen_margin)
{
    account->FrozenShortMargin += frozen_margin;
}

static void add_frozen_detail(zs_account_t* account, char* symbol, uint64_t sid,
    double margin, double price, int volume, char* userid)
{
    zs_frozen_detail_t* detail;
    detail = (zs_frozen_detail_t*)ztl_mp_alloc(account->FrozenDetailMP);
    detail->Margin = margin;
    detail->Price = price;
    detail->Volume = volume;
    detail->Sid = sid;
    strncpy(detail->Symbol, symbol, sizeof(detail->Symbol) - 1);
    strncpy(detail->UserID, userid, sizeof(detail->UserID) - 1);

    ztl_dlist_t* dlist;
    dlist = ztl_map_find(account->FrozenDetails, sid);
    if (!dlist)
    {
        dlist = ztl_dlist_create(8);
        ztl_map_add(account->FrozenDetails, sid, dlist);
    }
    ztl_dlist_insert_tail(dlist, detail);
}

static void update_frozen_detail(zs_account_t* account, char* symbol, uint64_t sid,
    double margin, double price, int volume, char* userid)
{
    zs_frozen_detail_t*     detail;
    ztl_dlist_iterator_t*   iter;
    ztl_dlist_t* dlist;
    
    dlist = ztl_map_find(account->FrozenDetails, sid);
    if (!dlist) {
        return;
    }

    int index = 0;
    zs_frozen_detail_t* founds[1024] = { 0 };

    iter = ztl_dlist_iter_new(dlist, ZTL_DLSTART_HEAD);
    while (true)
    {
        detail = ztl_dlist_next(dlist, iter);
        if (!detail) {
            break;
        }
        if (strcmp(detail->UserID, userid) != 0) {
            continue;
        }

        int residual = detail->Volume - volume;
        if (residual > 0) {
            detail->Volume = residual;
            detail->Margin -= margin;
            break;
        }
        else {
            volume -= detail->Volume;
            ztl_dlist_erase(dlist, iter);

            ztl_mp_free(account->FrozenDetailMP, detail);
        }
    }
}

static void update_frozen_margin(zs_account_t* account)
{
    double pre_frozen_margin = account->FrozenMargin;
    if (account->MaxMarginSide) {
        account->FrozenMargin = ztl_max(account->FrozenLongMargin, account->FrozenShortMargin);
    }
    else {
        account->FrozenMargin = account->FrozenLongMargin + account->FrozenShortMargin;
    }

    double margin_diff = account->FrozenMargin - pre_frozen_margin;
    account->FrozenMargin += margin_diff;
}

static void update_use_margin(zs_account_t* account, double margin)
{
    account->FundAccount.Margin += margin;
}

static void update_frozen_cash(zs_account_t* account, double cash)
{
    account->FundAccount.FrozenCash += cash;
}

static void update_avail_cash(zs_account_t* account, double cash)
{
    account->FundAccount.Available += cash;
}

static int freeze_margin(zs_account_t* account, char* symbol, uint64_t sid, ZSDirection direction,
    double price, int volume, char* userid, zs_contract_t* contract, double* pmargin)
{
    // 冻结保证金
    double frozen_margin = 0.0;
    if (direction == ZS_D_Long)
    {
        frozen_margin = calculate_margin(price, volume, contract->Multiplier, contract->OpenRatioByMoney);
        if (frozen_margin > account->FundAccount.Available) {
            // ERRORID: 可用资金不够
            return -1;
        }

        update_long_frozen_margin(account, frozen_margin);
    }
    else if (direction == ZS_D_Short)
    {
        frozen_margin = calculate_margin(price, volume, contract->Multiplier, contract->OpenRatioByMoney);
        if (frozen_margin > account->FundAccount.Available) {
            // ERRORID: 可用资金不够
            return -1;
        }

        update_short_frozen_margin(account, frozen_margin);
    }
    else
    {
        // ERRORID: 不合法的买卖标志
        return -2;
    }

    if (pmargin)
        *pmargin = frozen_margin;

    add_frozen_detail(account, symbol, sid, frozen_margin, price, volume, userid);
    update_frozen_margin(account);
    update_frozen_cash(account, frozen_margin);
    update_avail_cash(account, -frozen_margin);
    return 0;
}

static void free_frozen_margin(zs_account_t* account, char* symbol, uint64_t sid, ZSDirection direction,
    double price, int volume, char* userid, zs_contract_t* contract, double* pmargin)
{
    // 释放保证金
    double frozen_margin = 0.0;
    if (direction == ZS_D_Long)
    {
        frozen_margin = calculate_margin(price, volume, contract->Multiplier, contract->OpenRatioByMoney);
        update_long_frozen_margin(account, -frozen_margin);
    }
    else if (direction == ZS_D_Short)
    {
        frozen_margin = calculate_margin(price, volume, contract->Multiplier, contract->OpenRatioByMoney);
        update_short_frozen_margin(account, -frozen_margin);
    }
    else
    {
        return;
    }

    if (pmargin)
        *pmargin = frozen_margin;

    update_frozen_detail(account, symbol, sid, frozen_margin, price, volume, userid);
    update_frozen_margin(account);
    update_frozen_cash(account, -frozen_margin);
    update_avail_cash(account, frozen_margin);
}

static int freeze_commission(zs_account_t* account, ZSOffsetFlag offset, double price, int volume, zs_contract_t* contract)
{
    return 0;
}
static void free_frozen_commission(zs_account_t* account, ZSOffsetFlag offset, double price, int volume, zs_contract_t* contract)
{
    return ;
}


zs_account_t* zs_account_create(ztl_pool_t* pool)
{
    zs_account_t* account;
    if (pool) {
        account = (zs_account_t*)ztl_palloc(pool, sizeof(zs_account_t));
        memset(account, 0, sizeof(zs_account_t));
        account->Pool = pool;
    }
    else {
        account = (zs_account_t*)malloc(sizeof(zs_account_t));
        memset(account, 0, sizeof(zs_account_t));
    }

    account->AccountID = account->FundAccount.AccountID;
    account->FrozenDetails = ztl_map_create(16);
    account->FrozenDetailMP = ztl_mp_create(sizeof(zs_frozen_detail_t), 16, 1);
    return account;
}


static void _frozen_detail_map_access(ztl_map_t* pmap, 
    void* context1, int context2, uint64_t key, void* value)
{
    ztl_dlist_t* dlist;
    dlist = (ztl_dlist_t*)value;
    if (!value) {
        return;
    }

    // no need release each element

    ztl_dlist_release(dlist);
}

void zs_account_release(zs_account_t* account)
{
    if (!account) {
        return;
    }

    if (account->FrozenDetails) {
        ztl_map_traverse(account->FrozenDetails, _frozen_detail_map_access, account, 0);
        ztl_map_release(account->FrozenDetails);
        account->FrozenDetails = NULL;
    }

    if (account->FrozenDetailMP) {
        ztl_mp_release(account->FrozenDetailMP);
        account->FrozenDetailMP = NULL;
    }

    if (!account->Pool) {
        free(account);
    }
}

void zs_account_update(zs_account_t* account, zs_fund_account_t* fund_account)
{
    account->FundAccount = *fund_account;
}

int zs_account_on_order_req(zs_account_t* account, zs_order_req_t* order_req, zs_contract_t* contract)
{
    int rv;
    double frozen_margin;

    if (order_req->Offset != ZS_OF_Open)
    {
        return 0;
    }

    // 冻结资金
    rv = freeze_margin(account, order_req->Symbol, order_req->Sid, 
        order_req->Direction, order_req->Price, order_req->Quantity, 
        order_req->UserID, contract, &frozen_margin);
    if (rv != 0)
    {
        // ERRORID:
    }

    rv = freeze_commission(account, order_req->Offset, order_req->Price, order_req->Quantity, contract);
    if (rv != 0)
    {
        double frozen_margin = 0;
        free_frozen_margin(account, order_req->Symbol, order_req->Sid, order_req->Direction,
            order_req->Price, order_req->Quantity, order_req->UserID, order_req->Contract, &frozen_margin);

        // ERRORID: 手续费不够
        return rv;
    }

    return rv;
}

int zs_account_on_order_rtn(zs_account_t* account, zs_order_t* order, zs_contract_t* contract)
{
    int vol = order->Quantity - order->Filled;
    if (vol <= 0) {
        // do not process the order filled
        return 0;
    }

    if (order->Offset == ZS_OF_Open)
    {
        double frozen_margin = 0;
        free_frozen_margin(account, order->Symbol, order->Sid, order->Direction,
            order->Price, vol, order->UserID, contract, &frozen_margin);
    }
    free_frozen_commission(account, order->Offset, order->Price, vol, contract);

    return 0;
}

int zs_account_on_trade_rtn(zs_account_t* account, zs_order_t* order, zs_trade_t* trade, zs_contract_t* contract)
{
    double  margin, margin_ratio;
    double  price;
    int     volume;

    margin = 0;
    price = trade->Price;
    volume = trade->Volume;

    if (order->Direction == ZS_D_Long)
        margin_ratio = contract->LongMarginRateByMoney;
    else
        margin_ratio = contract->ShortMarginRateByMoney;
    margin = calculate_margin(price, volume, contract->Multiplier, margin_ratio);

    if (order->Offset == ZS_OF_Open)
    {
        // 开仓，重新计算保证金
        double frozen_margin = 0;
        free_frozen_margin(account, order->Symbol, order->Sid, order->Direction,
            order->Price, volume, order->UserID, contract, &frozen_margin);

        update_use_margin(account, margin);
        update_avail_cash(account, -margin);
    }
    else
    {
        update_use_margin(account, -margin);
        update_avail_cash(account, margin);
    }

    free_frozen_commission(account, order->Offset, order->Price, volume, contract);
    account->FundAccount.Commission += trade->Commission;
    update_avail_cash(account, -trade->Commission);

    return 0;
}

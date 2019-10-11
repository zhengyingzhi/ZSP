#include <ZToolLib/ztl_dlist.h>
#include <ZToolLib/ztl_utils.h>
#include <ZToolLib/ztl_palloc.h>

#include "zs_account.h"
#include "zs_blotter.h"
#include "zs_error.h"


static double calculate_margin(double price, int volume, int multiplier, double margin_ratio)
{
    return price * volume * multiplier * margin_ratio;
}

static void update_frozen_margin_long(zs_account_t* account, double frozen_margin)
{
    account->FrozenLongMargin += frozen_margin;
}

static void update_frozen_margin_short(zs_account_t* account, double frozen_margin)
{
    account->FrozenShortMargin += frozen_margin;
}

static void add_frozen_detail(zs_account_t* account, char* symbol, zs_sid_t sid,
    double margin, double price, int volume, char* userid)
{
    zs_frozen_detail_t* detail;
    detail = (zs_frozen_detail_t*)ztl_mp_alloc(account->FrozenDetailMP);
    detail->Sid     = sid;
    detail->Margin  = margin;
    detail->Price   = price;
    detail->Volume  = volume;
    strncpy(detail->Symbol, symbol, sizeof(detail->Symbol) - 1);
    strncpy(detail->UserID, userid, sizeof(detail->UserID) - 1);

    ztl_dlist_t* dlist;
    dlist = ztl_map_find(account->FrozenDetails, (uint64_t)sid);
    if (!dlist)
    {
        dlist = ztl_dlist_create(8);
        ztl_map_add(account->FrozenDetails, (uint64_t)sid, dlist);
    }
    ztl_dlist_insert_tail(dlist, detail);
}

static void update_frozen_detail(zs_account_t* account, char* symbol, zs_sid_t sid,
    double margin, double price, int volume, char* userid)
{
    zs_frozen_detail_t*     detail;
    ztl_dlist_iterator_t*   iter;
    ztl_dlist_t* dlist;
    
    dlist = ztl_map_find(account->FrozenDetails, (uint64_t)sid);
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
    if (account->MaxMarginSide) {
        account->FrozenMargin = ztl_max(account->FrozenLongMargin, account->FrozenShortMargin);
    }
    else {
        account->FrozenMargin = account->FrozenLongMargin + account->FrozenShortMargin;
    }
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

static int freeze_margin(zs_account_t* account, char* symbol, zs_sid_t sid, ZSDirection direction,
    double price, int volume, char* userid, zs_contract_t* contract, double* pmargin)
{
    // 冻结保证金
    double frozen_margin = 0.0;
    double margin_ratio = (direction == ZS_D_Long) ? contract->LongMarginRateByMoney : contract->ShortMarginRateByMoney;
    frozen_margin = calculate_margin(price, volume, contract->Multiplier, margin_ratio);
    if (frozen_margin > account->FundAccount.Available) {
        return ZS_ERR_AvailCash;
    }

    if (direction == ZS_D_Long) {
        update_frozen_margin_long(account, frozen_margin);
    }
    else if (direction == ZS_D_Short) {
        update_frozen_margin_short(account, frozen_margin);
    }
    else {
        return ZS_ERR_InvalidField;
    }

    add_frozen_detail(account, symbol, sid, frozen_margin, price, volume, userid);
    update_frozen_margin(account);
    update_frozen_cash(account, frozen_margin);
    update_avail_cash(account, -frozen_margin);

    if (pmargin) {
        *pmargin = frozen_margin;
    }

    return ZS_OK;
}

static void free_frozen_margin(zs_account_t* account, char* symbol, zs_sid_t sid, ZSDirection direction,
    double price, int volume, char* userid, zs_contract_t* contract, double* pmargin)
{
    // 释放保证金
    double frozen_margin = 0.0;
    double margin_ratio = (direction == ZS_D_Long) ? contract->LongMarginRateByMoney : contract->ShortMarginRateByMoney;
    frozen_margin = calculate_margin(price, volume, contract->Multiplier, margin_ratio);

    if (direction == ZS_D_Long) {
        update_frozen_margin_long(account, -frozen_margin);
    }
    else if (direction == ZS_D_Short) {
        update_frozen_margin_short(account, -frozen_margin);
    }
    else {
        return;
    }

    if (pmargin) {
        *pmargin = frozen_margin;
    }

    update_frozen_detail(account, symbol, sid, frozen_margin, price, volume, userid);
    update_frozen_margin(account);
    update_frozen_cash(account, -frozen_margin);
    update_avail_cash(account, frozen_margin);
}

static int freeze_commission(zs_account_t* account, ZSOffsetFlag offset, double price, int volume, zs_contract_t* contract)
{
    // not impl yet
    return ZS_OK;
}

static void free_frozen_commission(zs_account_t* account, ZSOffsetFlag offset, double price, int volume, zs_contract_t* contract)
{
    return ;
}


zs_account_t* zs_account_create(zs_blotter_t* blotter)
{
    zs_account_t* account;
    if (blotter) {
        account = (zs_account_t*)ztl_pcalloc(blotter->Pool, sizeof(zs_account_t));
        account->Pool = blotter->Pool;
    }
    else {
        account = (zs_account_t*)malloc(sizeof(zs_account_t));
        memset(account, 0, sizeof(zs_account_t));
    }

    account->AccountID = account->FundAccount.AccountID;
    account->Blotter = blotter;
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

void zs_account_fund_update(zs_account_t* account, zs_fund_account_t* fund_account)
{
    account->FundAccount = *fund_account;
}

int zs_account_handle_order_req(zs_account_t* account, zs_order_req_t* order_req, zs_contract_t* contract)
{
    int rv;
    double frozen_margin;

    if (order_req->OffsetFlag != ZS_OF_Open) {
        return ZS_OK;
    }

    // 冻结资金
    rv = freeze_margin(account, order_req->Symbol, order_req->Sid, 
        order_req->Direction, order_req->OrderPrice, order_req->OrderQty,
        order_req->UserID, contract, &frozen_margin);
    if (rv != ZS_OK) {
        return rv;
    }

    zs_log_info(account->Blotter->Log, "account: handle_order_req freeze margin:%.2lf", frozen_margin);

    rv = freeze_commission(account, order_req->OffsetFlag, order_req->OrderPrice, order_req->OrderQty, contract);
    if (rv != ZS_OK)
    {
        // 手续费不够,释放刚才冻结的保证金
        free_frozen_margin(account, order_req->Symbol, order_req->Sid, order_req->Direction,
            order_req->OrderPrice, order_req->OrderQty, order_req->UserID, contract, &frozen_margin);
    }

    return rv;
}

int zs_account_handle_order_finished(zs_account_t* account, zs_order_t* order, zs_contract_t* contract)
{
    // 处理完成状态（不包括完全成交）
    int vol = order->OrderQty - order->FilledQty;
    if (vol <= 0) {
        return ZS_OK;
    }

    if (order->OffsetFlag == ZS_OF_Open)
    {
        // FIXME: 只有期权卖方才会冻结保证金
        double frozen_margin = 0;
        free_frozen_margin(account, order->Symbol, order->Sid, order->Direction,
            order->OrderPrice, vol, order->UserID, contract, &frozen_margin);
        zs_log_info(account->Blotter->Log, "handle_order_finished free margin:%.2lf, account:%s, symbol:%s",
            frozen_margin, account->AccountID, order->Symbol);
    }
    free_frozen_commission(account, order->OffsetFlag, order->OrderPrice, vol, contract);

    return ZS_OK;
}

int zs_account_handle_trade_rtn(zs_account_t* account, zs_order_t* order, zs_trade_t* trade, zs_contract_t* contract)
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

    if (order->OffsetFlag == ZS_OF_Open)
    {
        // 开仓，重新计算保证金占用
        double frozen_margin = 0;
        free_frozen_margin(account, order->Symbol, order->Sid, order->Direction,
            order->OrderPrice, volume, order->UserID, contract, &frozen_margin);

        zs_log_info(account->Blotter->Log, "handle_trade_rtn free margin:%.2lf, used_margin:%.2lf, account:%s, symbol:%s",
            frozen_margin, margin, account->AccountID, order->Symbol);

        update_use_margin(account, margin);
        update_avail_cash(account, -margin);
    }
    else
    {
        // 平仓，释放保证金占用
        update_use_margin(account, -margin);
        update_avail_cash(account, margin);
    }

    free_frozen_commission(account, order->OffsetFlag, order->OrderPrice, volume, contract);
    update_avail_cash(account, -trade->Commission);
    account->FundAccount.Commission += trade->Commission;

    return ZS_OK;
}

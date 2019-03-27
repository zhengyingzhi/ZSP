#include "zs_order_list.h"


zs_orderlist_t* zs_orderlist_create()
{
    zs_orderlist_t* order_list;
    order_list = ztl_dlist_create(32);
    return 0;
}

void zs_orderlist_release(zs_orderlist_t* orderList)
{
    if (orderList)
        ztl_dlist_release(orderList);
}

int zs_orderlist_append(zs_orderlist_t* orderList, zs_order_t* pOrder)
{
    return ztl_dlist_insert_tail(orderList, pOrder);
}

int zs_orderlist_remove(zs_orderlist_t* orderList, zs_order_t* pOrder)
{
    ztl_dlist_iterator_t* iter;
    iter = ztl_dlist_iter_new(orderList, ZTL_DLSTART_HEAD);
    while (iter)
    {
        zs_order_t* lpOrd;
        lpOrd = (zs_order_t*)iter->nodelink;    // TODO: we should get iter's data
        if (lpOrd == pOrder) {
            //ztl_dlist_remove(iter->node);
            break;
        }

        iter = ztl_dlist_next(orderList, iter);
    }

    ztl_dlist_iter_del(orderList, iter);
    return 0;
}

zs_order_t* zs_order_find(zs_orderlist_t* orderList, ZSExchangeID exchangeID,
    const char aOrderSysID[])
{
    zs_order_t* order = NULL;
    ztl_dlist_iterator_t* iter;
    iter = ztl_dlist_iter_new(orderList, ZTL_DLSTART_HEAD);
    while (iter)
    {
        zs_order_t* temp;
        temp = (zs_order_t*)iter->nodelink;    // TODO: we should get iter's data
        if (temp->ExchangeID == exchangeID && strcmp(temp->OrderSysID, aOrderSysID) == 0) {
            order = temp;
            break;
        }

        iter = ztl_dlist_next(orderList, iter);
    }

    ztl_dlist_iter_del(orderList, iter);
    return order;
}

#include "zs_order_list.h"


zs_orderlist_t* zs_orderlist_create()
{
    zs_orderlist_t* order_list;
    order_list = ztl_dlist_create(32);
    return 0;
}

void zs_orderlist_release(zs_orderlist_t* orderlist)
{
    if (orderlist)
        ztl_dlist_release(orderlist);
}

int zs_orderlist_append(zs_orderlist_t* orderlist, zs_order_t* order)
{
    return ztl_dlist_insert_tail(orderlist, order);
}

int zs_orderlist_remove(zs_orderlist_t* orderlist, zs_order_t* order)
{
    ztl_dlist_iterator_t* iter;
    iter = ztl_dlist_iter_new(orderlist, ZTL_DLSTART_HEAD);
    while (iter)
    {
        zs_order_t* lorder;
        lorder = (zs_order_t*)iter->nodelink;    // TODO: we should get iter's data
        if (lorder == order) {
            //ztl_dlist_remove(iter->node);
            break;
        }

        iter = ztl_dlist_next(orderlist, iter);
    }

    ztl_dlist_iter_del(orderlist, iter);
    return 0;
}

zs_order_t* zs_order_find(zs_orderlist_t* orderlist, int32_t frontid, int32_t sessionid,
    const char orderid[])
{
    zs_order_t* order = NULL;
    ztl_dlist_iterator_t* iter;
    iter = ztl_dlist_iter_new(orderlist, ZTL_DLSTART_HEAD);
    while (iter)
    {
        zs_order_t* temp;
        temp = (zs_order_t*)iter->nodelink;    // TODO: we should get iter's data
        if (temp->FrontID == frontid && temp->SessionID == sessionid && strcmp(temp->OrderID, orderid) == 0) {
            order = temp;
            break;
        }

        iter = ztl_dlist_next(orderlist, iter);
    }

    ztl_dlist_iter_del(orderlist, iter);
    return order;
}

zs_order_t* zs_order_find_by_sysid(zs_orderlist_t* orderlist, ZSExchangeID exchangeid,
    const char order_sysid[])
{
    zs_order_t* order = NULL;
    ztl_dlist_iterator_t* iter;
    iter = ztl_dlist_iter_new(orderlist, ZTL_DLSTART_HEAD);
    while (iter)
    {
        zs_order_t* temp;
        temp = (zs_order_t*)iter->nodelink;    // TODO: we should get iter's data
        if (temp->ExchangeID == exchangeid && strcmp(temp->OrderSysID, order_sysid) == 0) {
            order = temp;
            break;
        }

        iter = ztl_dlist_next(orderlist, iter);
    }

    ztl_dlist_iter_del(orderlist, iter);
    return order;
}

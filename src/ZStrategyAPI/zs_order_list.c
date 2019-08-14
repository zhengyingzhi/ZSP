#include <ZToolLib/ztl_memcpy.h>

#include "zs_order_list.h"


zs_orderlist_t* zs_orderlist_create()
{
    zs_orderlist_t* order_list;
    order_list = ztl_dlist_create(32);
    return order_list;
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
    int rv;
    ztl_dlist_iterator_t* iter;

    rv = -1;
    iter = ztl_dlist_iter_new(orderlist, ZTL_DLSTART_HEAD);
    do
    {
        zs_order_t* temp;
        temp = ztl_dlist_next(orderlist, iter);
        if (!temp) {
            break;
        }

        if (order == temp)
        {
            ztl_dlist_erase(orderlist, iter);
            rv = 0;
            break;
        }
    } while (1);

    ztl_dlist_iter_del(orderlist, iter);
    return rv;
}


int zs_orderlist_size(zs_orderlist_t* orderlist)
{
    return ztl_dlist_size(orderlist);
}

int zs_orderlist_retrieve(zs_orderlist_t* orderlist, zs_order_t* orders[], int size, uint64_t filter_sid)
{
    int index = 0;
    zs_order_t* order = NULL;
    ztl_dlist_iterator_t* iter;

    iter = ztl_dlist_iter_new(orderlist, ZTL_DLSTART_HEAD);
    do
    {
        zs_order_t* temp;
        temp = ztl_dlist_next(orderlist, iter);
        if (!temp) {
            break;
        }
        else if (index >= size) {
            break;
        }

        if (!filter_sid || temp->Sid == filter_sid) {
            orders[index++] = temp;
        }
    } while (1);

    ztl_dlist_iter_del(orderlist, iter);
    return index;
}


zs_order_t* zs_order_find(zs_orderlist_t* orderlist, int32_t frontid, int32_t sessionid,
    const char orderid[])
{
    zs_order_t* order = NULL;
    zs_order_t* temp;
    ztl_dlist_iterator_t* iter;

    iter = ztl_dlist_iter_new(orderlist, ZTL_DLSTART_HEAD);
    do
    {
        temp = ztl_dlist_next(orderlist, iter);
        if (!temp) {
            break;
        }

        if (temp->FrontID == frontid && temp->SessionID == sessionid &&
            strcmp(temp->OrderID, orderid) == 0) {
            order = temp;
            break;
        }
    } while (1);

    ztl_dlist_iter_del(orderlist, iter);
    return order;
}

zs_order_t* zs_order_find_by_sysid(zs_orderlist_t* orderlist, ZSExchangeID exchangeid,
    const char order_sysid[])
{
    zs_order_t* order = NULL;
    ztl_dlist_iterator_t* iter;
    iter = ztl_dlist_iter_new(orderlist, ZTL_DLSTART_HEAD);
    do
    {
        order = ztl_dlist_next(orderlist, iter);
        if (!order) {
            break;
        }

        if (order->ExchangeID == exchangeid && strcmp(order->OrderSysID, order_sysid) == 0) {
            break;
        }
    } while (1);

    ztl_dlist_iter_del(orderlist, iter);
    return order;
}


//////////////////////////////////////////////////////////////////////////

static uint64_t _zs_order_keyhash(const void *key) {
    ZSOrderKey* skey = (ZSOrderKey*)key;
    return dictGenHashFunction((unsigned char*)skey->pOrderID, skey->Length);
}

static void* _zs_order_keydup(void* priv, const void* key) {
    ztl_pool_t* pool = (ztl_pool_t*)priv;
    ZSOrderKey* skey = (ZSOrderKey*)key;
    ZSOrderKey* dup_key = (ZSOrderKey*)ztl_palloc(pool, ztl_align(sizeof(ZSOrderKey) + skey->Length + 1, 4));
    dup_key->SessionID = skey->SessionID;
    dup_key->FrontID = skey->FrontID;
    dup_key->Length = skey->Length;
    dup_key->pOrderID = (char*)(dup_key + 1);
    dup_key->pOrderID[skey->Length] = '\0';
    ztl_memcpy(dup_key->pOrderID, skey->pOrderID, skey->Length);  // could be use a faster copy
    return dup_key;
}

static void* _zs_order_valdup(void* priv, const void* obj) {
    ztl_pool_t* pool = (ztl_pool_t*)priv;
    zs_order_t* dupord = ztl_palloc(pool, sizeof(zs_order_t));
    ztl_memcpy(dupord, obj, sizeof(zs_order_t));
    return dupord;
}

static int _zs_order_keycmp(void* priv, const void* s1, const void* s2) {
    (void)priv;
    ZSOrderKey* k1 = (ZSOrderKey*)s1;
    ZSOrderKey* k2 = (ZSOrderKey*)s2;
    return (k1->SessionID == k2->SessionID) \
        && (k1->FrontID == k2->FrontID) \
        && memcmp(k1->pOrderID, k2->pOrderID, k2->Length) == 0;
}

static dictType orderHashDictType = {
    _zs_order_keyhash,
    _zs_order_keydup,
    NULL,       // _zs_order_valdup,
    _zs_order_keycmp,
    NULL,
    NULL
};


zs_orderdict_t* zs_orderdict_create(ztl_pool_t* privptr)
{
    zs_orderdict_t* orderdict;
    orderdict = dictCreate(&orderHashDictType, privptr);
    return orderdict;
}

void zs_orderdict_release(zs_orderdict_t* orderdict)
{
    dictRelease(orderdict);
}

void zs_orderdict_clear(zs_orderdict_t* orderdict, void (callback)(void*))
{
    dictEmpty(orderdict, callback);
}

int zs_orderdict_add_order(zs_orderdict_t* orderdict, zs_order_t* order)
{
    ZSOrderKey key;

    key.SessionID = order->SessionID;
    key.FrontID = order->FrontID;
    key.Length = (int)strlen(order->OrderID);
    key.pOrderID = order->OrderID;

    return dictAdd(orderdict, &key, order);
}

int zs_orderdict_add(zs_orderdict_t* orderdict, int32_t frontid, int32_t sessionid,
    const char orderid[], void* value)
{
    ZSOrderKey key;

    key.SessionID = sessionid;
    key.FrontID = frontid;
    key.Length = (int)strlen(orderid);
    key.pOrderID = (char*)orderid;

    return dictAdd(orderdict, &key, value);
}

int zs_orderdict_del(zs_orderdict_t* orderdict, int32_t frontid, int32_t sessionid,
    const char orderid[])
{
    return 0;
}

int zs_orderdict_size(zs_orderdict_t* orderdict)
{
    return dictSize(orderdict);
}

int zs_orderdict_retrieve(zs_orderdict_t* orderdict, zs_order_t* orders[], int size)
{
    return 0;
}

void* zs_orderdict_find(zs_orderdict_t* orderdict, int32_t frontid, int32_t sessionid,
    const char orderid[])
{
    dictEntry*  entry;
    ZSOrderKey  key;
    key.SessionID = sessionid;
    key.FrontID = frontid;
    key.Length = (int)strlen(orderid);
    key.pOrderID = (char*)orderid;

    entry = dictFind(orderdict, &key);
    if (entry) {
        return (void*)entry->v.val;
    }

    return NULL;
}

void* zs_orderdict_find_by_sysid(zs_orderdict_t* orderdict, ZSExchangeID exchangeid,
    const char order_sysid[])
{
    return NULL;
}

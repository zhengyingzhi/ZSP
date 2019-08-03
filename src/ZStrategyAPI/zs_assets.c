
#include <ZToolLib/ztl_hash.h>

#include "zs_api_object.h"
#include "zs_constants.h"
#include "zs_assets.h"
#include "zs_hashdict.h"


zs_sid_t zs_asset_sid_gen(zs_asset_finder_t* asset_finder, 
    int exchangeid, const char* symbol, int len);
zs_sid_t zs_asset_sid_get(zs_asset_finder_t* asset_finder, 
    int exchangeid, const char* symbol, int len);


typedef struct
{
    uint16_t flag;      // usually exchange id
    uint16_t len;       // symbol length
    char*    ptr;       // symbol ptr
}ZAssetKey;


static uint64_t _zs_asset_hash(const void *key) {
    ZAssetKey* skey = (ZAssetKey*)key;
    return dictGenHashFunction((unsigned char*)skey->ptr, skey->len);
}

static int _zs_asset_cmp(void* priv, const void* a1, const void* a2) {
    (void)priv;
    ZAssetKey* k1 = (ZAssetKey*)a1;
    ZAssetKey* k2 = (ZAssetKey*)a2;
    return (k1->flag == k2->flag) && memcmp(k1->ptr, k2->ptr, k2->len) == 0;
}

static void* _zs_asset_keydup(void* priv, const void* key) {
    zs_asset_finder_t* asset_finder = (zs_asset_finder_t*)priv;
    ZAssetKey* skey = (ZAssetKey*)key;
    ZAssetKey* dup_key = (ZAssetKey*)ztl_palloc(asset_finder->Pool,
        ztl_align(sizeof(ZAssetKey) + skey->len, 4));
    dup_key->flag = skey->flag;
    dup_key->len = skey->len;
    dup_key->ptr = (char*)(dup_key + 1);
    memcpy(dup_key->ptr, skey->ptr, skey->len);
    return dup_key;
}

static dictType assetHashDictType = {
    _zs_asset_hash,
    _zs_asset_keydup,
    NULL,
    _zs_asset_cmp,
    NULL,
    NULL
};

zs_asset_finder_t* zs_asset_create(void* ctxdata, ztl_pool_t* pool, int init_num)
{
    zs_asset_finder_t* asset_finder;

    int CreatedSelf = 0;
    if (!pool)
    {
        pool = ztl_create_pool(ZTL_DEFAULT_POOL_SIZE);
        CreatedSelf = 1;
    }
    asset_finder = (zs_asset_finder_t*)ztl_pcalloc(pool, ztl_align(sizeof(zs_asset_finder_t), 8));

    asset_finder->ContextData = ctxdata;
    asset_finder->Pool = pool;

    // sid hash table
    asset_finder->SymbolHashDict = dictCreate(&assetHashDictType, asset_finder);

    // asset table by sid index
    (void)init_num;
    asset_finder->AssetTable = dictCreate(&uintHashDictType, asset_finder);

    asset_finder->BaseSid = ZS_ASSET_START_SID;

    asset_finder->CreatedSelf = CreatedSelf;
    asset_finder->EnableDefaultEquity = 1;
    asset_finder->Count = 0;

    return asset_finder;
}

void zs_asset_release(zs_asset_finder_t* asset_finder)
{
    if (asset_finder)
    {
        if (asset_finder->SymbolHashDict) {
            dictRelease(asset_finder->SymbolHashDict);
            asset_finder->SymbolHashDict = NULL;
        }

        if (asset_finder->AssetTable) {
            dictRelease(asset_finder->AssetTable);
            asset_finder->AssetTable = NULL;
        }

        if (asset_finder->CreatedSelf) {
            ztl_destroy_pool(asset_finder->Pool);
        }
    }
}


int zs_asset_enable_default_equity(zs_asset_finder_t* asset_finder, int32_t enable)
{
    asset_finder->EnableDefaultEquity = enable;
    return 0;
}

int zs_asset_add(zs_asset_finder_t* asset_finder, zs_sid_t* psid,
    int exchangeid, const char* symbol, int len, void* data)
{
    zs_sid_t sid;
    void* dst;

    sid = zs_asset_sid_gen(asset_finder, exchangeid, symbol, len);
    if (sid == ZS_SID_INVALID) {
        return -1;
    }

    dst = zs_asset_find_by_sid(asset_finder, sid);
    if (!dst) {
        dictAdd(asset_finder->AssetTable, (void*)sid, data);
    }
    else {
        // already exists
        *psid = sid;
        return 1;
    }

    *psid = sid;

    asset_finder->Count += 1;

    return 0;
}

int zs_asset_add_copy(zs_asset_finder_t* asset_finder, zs_sid_t* psid,
    int exchangeid, const char* symbol, int len, void* data, int size)
{
    zs_sid_t sid;
    void* dst;

    sid = zs_asset_sid_gen(asset_finder, exchangeid, symbol, len);
    if (sid == ZS_SID_INVALID) {
        return -1;
    }

    dst = zs_asset_find_by_sid(asset_finder, sid);
    if (!dst)
    {
        // do a copy
        dst = ztl_palloc(asset_finder->Pool, ztl_align(size, 8));
        memcpy(dst, data, size);

        dictAdd(asset_finder->AssetTable, (void*)sid, dst);
    }
    else
    {
        *psid = sid;
        return 1;
    }

    *psid = sid;

    asset_finder->Count += 1;

    return 0;
}

int zs_asset_del(zs_asset_finder_t* asset_finder, int exchangeid, const char* symbol, int len)
{
    zs_sid_t sid;
    sid = zs_asset_lookup(asset_finder, exchangeid, symbol, len);
    if (sid == ZS_SID_INVALID)
    {
        return -1;
    }

    // dictDelete(asset_finder->SymbolHashDict, dst->Symbol);
    return zs_asset_del_by_sid(asset_finder, sid);
}

int zs_asset_del_by_sid(zs_asset_finder_t* asset_finder, zs_sid_t sid)
{
    void* dst;

    dst = zs_asset_find_by_sid(asset_finder, sid);
    if (dst)
    {
        dictDelete(asset_finder->AssetTable, (void*)sid);

        asset_finder->Count -= 1;

        // here not release the dst object
        return 0;
    }
    return -1;
}

zs_sid_t zs_asset_lookup(zs_asset_finder_t* asset_finder, int exchangeid, const char* symbol, int len)
{
    zs_sid_t sid;
    sid = zs_asset_sid_get(asset_finder, exchangeid, symbol, len);
    return sid;
}

void* zs_asset_find(zs_asset_finder_t* asset_finder, int exchangeid, const char* symbol, int len)
{
    zs_sid_t sid;

    sid = zs_asset_lookup(asset_finder, exchangeid, symbol, len);
    if (sid == ZS_SID_INVALID) {
        return NULL;
    }

    return zs_asset_find_by_sid(asset_finder, sid);
}

void* zs_asset_find_by_sid(zs_asset_finder_t* asset_finder, zs_sid_t sid)
{
    dictEntry* entry;
    entry = dictFind(asset_finder->AssetTable, (void*)sid);
    if (entry) {
        return entry->v.val;
    }

    return entry;
}

uint32_t zs_asset_count(zs_asset_finder_t* asset_finder)
{
    return asset_finder->Count;
}



zs_sid_t zs_asset_sid_gen(zs_asset_finder_t* asset_finder, 
    int exchangeid, const char* symbol, int len)
{
    zs_sid_t sid;
    sid = zs_asset_sid_get(asset_finder, exchangeid, symbol, len);
    if (sid == ZS_SID_INVALID)
    {
        // add new if not existed
        ZAssetKey key = { (uint16_t)exchangeid, (uint16_t)len, (char*)symbol };
        sid = asset_finder->BaseSid++;
        dictAdd(asset_finder->SymbolHashDict, &key, (void*)sid);
    }

    return sid;
}

zs_sid_t zs_asset_sid_get(zs_asset_finder_t* asset_finder, 
    int exchangeid, const char* symbol, int len)
{
    dictEntry*  entry;
    zs_sid_t    sid = ZS_SID_INVALID;
    ZAssetKey   key = { (uint16_t)exchangeid, (uint16_t)len, (char*)symbol };
    entry = dictFind(asset_finder->SymbolHashDict, &key);
    if (entry)
    {
        // found
        sid = (zs_sid_t)entry->v.u64;
    }

    if ((exchangeid == ZS_EI_SSE || exchangeid == ZS_EI_SZSE) &&
        asset_finder->EnableDefaultEquity)
    {
        sid = zs_asset_sid_gen(asset_finder, exchangeid, symbol, len);
        if (sid == ZS_SID_INVALID) {
            return -1;
        }

        zs_contract_t* contract;
        contract = ztl_pcalloc(asset_finder->Pool, sizeof(zs_contract_t));
        strcpy(contract->Symbol, symbol);
        contract->ExchangeID = exchangeid;
        contract->Sid = sid;
        contract->PriceTick = 0.01;     // FIXME AShare
        contract->ProductClass = ZS_PC_Stock;
        contract->Multiplier = 1;
        contract->Decimal = 2;
        contract->LongMarginRateByMoney = 1.0;
        contract->ShortMarginRateByMoney = 0.0;
        contract->OpenRatioByMoney = 0.0003;
        contract->CloseRatioByMoney = 0.0003;

        dictAdd(asset_finder->AssetTable, (void*)sid, contract);
    }

    return sid;
}

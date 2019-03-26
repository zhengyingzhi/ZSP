/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the assets(Equity/Future) manager object
 */

#ifndef _ZS_ASSETS_H_INCLUDED_
#define _ZS_ASSETS_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_dict.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_palloc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef dict ztl_dict_t;

typedef uint64_t zs_sid_t;

#define ZS_ASSET_INIT_NUM   8192
#define ZS_ASSET_START_SID  16

#define ZS_INVALID_SID      (-1)

/* global asset manager
 */
struct zs_asset_finder_s
{
    void*               ContextData;
    ztl_pool_t*         Pool;
    ztl_dict_t*         SymbolHashDict;     // <symbol, sid>
    ztl_dict_t*         AssetTable;         // Table<sid, assetinfo>
    zs_sid_t            BaseSid;            // start from ZS_ASSET_START_SID
    int32_t             CreatedSelf;
    uint32_t            Count;
};

typedef struct zs_asset_finder_s zs_asset_finder_t;

// create the asset finder, init_num is the reserved asset count
zs_asset_finder_t* zs_asset_create(void* ctxdata, ztl_pool_t* pool, int init_num);

// release the asset finder
void zs_asset_release(zs_asset_finder_t* asset_finder);

// add data, and will do a copy of it internally
int zs_asset_add(zs_asset_finder_t* asset_finder, zs_sid_t* psid, const char* symbol, int len, void* data);
int zs_asset_add_copy(zs_asset_finder_t* asset_finder, zs_sid_t* psid, const char* symbol, int len, void* data, int size);

// del data
int zs_asset_del(zs_asset_finder_t* asset_finder, const char* symbol, int len);
int zs_asset_del_by_sid(zs_asset_finder_t* asset_finder, zs_sid_t sid);

// lookup sid, str maybe the symbol
zs_sid_t zs_asset_lookup(zs_asset_finder_t* asset_finder, const char* str, int len);

// get data pointer which added before
void* zs_asset_find(zs_asset_finder_t* asset_finder, const char* symbol, int len);
void* zs_asset_find_by_sid(zs_asset_finder_t* asset_finder, zs_sid_t sid);

// get current count
uint32_t zs_asset_count(zs_asset_finder_t* asset_finder);


#ifdef __cplusplus
}
#endif

#endif//_ZS_ASSETS_H_INCLUDED_

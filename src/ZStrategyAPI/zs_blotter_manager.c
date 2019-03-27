#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_palloc.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_blotter_manager.h"

#include "zs_data_portal.h"

extern dictType sidHashDictType;

void zs_blotter_manager_init(zs_blotter_manager_t* blotterMgr, zs_algorithm_t* algo)
{
    blotterMgr->BlotterDict = dictCreate(&sidHashDictType, algo);
    ztl_array_init(&blotterMgr->BlotterArray, algo->Pool, 32, sizeof(zs_blotter_t*));
}

void zs_blotter_manager_add(zs_blotter_manager_t* blotterMgr, zs_blotter_t* blotter)
{
    char* account_id = blotter->Account->AccountID;

    dictAdd(blotterMgr->BlotterDict, account_id, blotter);

    ztl_array_push_back(&blotterMgr->BlotterArray, blotter);
}

zs_blotter_t* zs_blotter_manager_get(zs_blotter_manager_t* blotterMgr, const char* accountID)
{
    zs_blotter_t* blotter = NULL;

    if (!accountID || !accountID[0] || ztl_array_size(&blotterMgr->BlotterArray) == 1)
    {
        blotter = (zs_blotter_t*)ztl_array_at(&blotterMgr->BlotterArray, 0);
    }
    else
    {
        dictEntry* entry;
        entry = dictFind(blotterMgr->BlotterDict, accountID);
        if (entry)
            blotter = entry->v.val;
    }

    return blotter;
}


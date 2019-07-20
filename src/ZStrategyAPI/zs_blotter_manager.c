#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_palloc.h>

#include "zs_algorithm.h"

#include "zs_assets.h"

#include "zs_blotter_manager.h"

#include "zs_data_portal.h"

#include "zs_hashdict.h"


void zs_blotter_manager_init(zs_blotter_manager_t* manager, zs_algorithm_t* algo)
{
    manager->Algorithm = algo;
    manager->BlotterDict = dictCreate(&strHashDictType, algo);
    manager->BlotterArray = (ztl_array_t*)ztl_pcalloc(algo->Pool, sizeof(ztl_array_t));
    ztl_array_init(manager->BlotterArray, NULL, 32, sizeof(zs_blotter_t*));
}

void zs_blotter_manager_release(zs_blotter_manager_t* manager)
{
    if (manager->BlotterDict) {
        dictRelease(manager->BlotterDict);
        manager->BlotterDict = NULL;
    }

    if (manager->BlotterArray) {
        ztl_array_release(manager->BlotterArray);
        manager->BlotterArray = NULL;
    }
}

void zs_blotter_manager_add(zs_blotter_manager_t* manager, zs_blotter_t* blotter)
{
    int len = (int)strlen(blotter->Account->AccountID);

    ZStrKey* key = zs_str_keydup2(blotter->Account->AccountID, len, ztl_pcalloc, blotter->Algorithm->Pool);
    dictAdd(manager->BlotterDict, key, blotter);

    void** dst = ztl_array_push(manager->BlotterArray);
    *dst = blotter;
}

zs_blotter_t* zs_blotter_manager_get(zs_blotter_manager_t* manager, const char* accountid)
{
    zs_blotter_t* blotter = NULL;

    if (!accountid || !accountid[0] || ztl_array_size(manager->BlotterArray) == 1)
    {
        blotter = *(zs_blotter_t**)ztl_array_at(manager->BlotterArray, 0);
    }
    else
    {
        dictEntry* entry;
        entry = zs_strdict_find(manager->BlotterDict, accountid, (int)strlen(accountid));
        if (entry)
            blotter = entry->v.val;
    }

    return blotter;
}


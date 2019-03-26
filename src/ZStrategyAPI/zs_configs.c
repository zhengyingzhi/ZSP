#include <stdio.h>

#include <ZToolLib/ztl_config.h>

#include <ZToolLib/cJSON.h>

#include "zs_api_object.h"

#include "zs_configs.h"

#include "zs_core.h"


static char* _zs_pool_str_dup(ztl_pool_t* pool, const char* src, int length)
{
    if (length <= 0)
        length = (int)strlen(src);

    char* dst;
    dst = ztl_pcalloc(pool, ztl_align(length + 1, 8));
    memcpy(dst, src, length);
    return dst;
}

int zs_configs_load_json(zs_algo_param_t* algoParam, ztl_pool_t* pool, 
    const char* confFile);

int zs_configs_load(zs_algo_param_t* algoParam, ztl_pool_t* pool, 
    const char* confFile)
{
    if (strstr(confFile, ".json")) {
        return zs_configs_load_json(algoParam, pool, confFile);
    }

    return -1;
    strcpy(algoParam->StartDate, "2018-08-01");
    strcpy(algoParam->EndDate, "2018-08-03");

    //zs_account_t* account;
    //ztl_array_init(&algoParam->Accounts, NULL, 8, sizeof(zs_account_t));
    //account = ztl_array_push(&algoParam->Accounts);
    //strcpy(account->AccountID, "000100000002");
    //account->Available = 1000000.0;
    //account->Balance = 1000000.0;
    //account->UniqueID = 1;

    algoParam->RunMode = ZS_RM_Backtest;
    algoParam->DataFrequency = ZS_DF_Daily;
    algoParam->ProductType = ZS_PC_Stock;
    algoParam->CapitalBase = 1000000;

    return 0;
}

static int _zs_get_string_object_value(cJSON* tnode, const char* key, char* dst, 
    int dstSize)
{
    cJSON* obj;
    obj = cJSON_GetObjectItem(tnode, key);
    if (obj) {
        strncpy(dst, obj->valuestring, dstSize);
        return 0;
    }
    return -1;
}

static int _zs_configs_parse_tradings(cJSON* tnode, ztl_pool_t* pool, 
    zs_trading_conf_t* tradingConf)
{
    int rv;
    cJSON* obj;
    zs_broker_conf_t *ptd_conf, *pmd_conf;

    /* api confs */
    ptd_conf = &tradingConf->TradeConf;
    pmd_conf = &tradingConf->MdConf;

    rv = _zs_get_string_object_value(tnode, "AccountID", ptd_conf->UserID, sizeof(ptd_conf->UserID) - 1);
    if (0 != rv) {
        return rv;
    }
    _zs_get_string_object_value(tnode, "Password", ptd_conf->Password, sizeof(ptd_conf->Password) - 1);
    _zs_get_string_object_value(tnode, "BrokerID", ptd_conf->BrokerID, sizeof(ptd_conf->BrokerID) - 1);
    _zs_get_string_object_value(tnode, "TradeAPIName", ptd_conf->ApiName, sizeof(ptd_conf->ApiName) - 1);
    _zs_get_string_object_value(tnode, "TradeAddr", ptd_conf->Addr, sizeof(ptd_conf->Addr) - 1);

    ptd_conf->AuthCode = NULL;
    if (cJSON_HasObjectItem(tnode, "AuthCode"))
    {
        cJSON* obj;
        obj = cJSON_GetObjectItem(tnode, "AuthCode");
        ptd_conf->AuthCode = _zs_pool_str_dup(pool, obj->valuestring, -1);
    }

    memcpy(pmd_conf, ptd_conf, sizeof(zs_broker_conf_t));
    _zs_get_string_object_value(tnode, "MdAPIName", pmd_conf->ApiName, sizeof(pmd_conf->ApiName) - 1);
    _zs_get_string_object_value(tnode, "MdAddr", pmd_conf->Addr, sizeof(pmd_conf->Addr) - 1);

    /* strategy confs */
    ztl_array_init(&tradingConf->StrategyArr, pool, 8, sizeof(zs_strategy_conf_t));

    obj = cJSON_GetObjectItem(tnode, "Strategy");
    if (obj)
    {
        if (obj->type != cJSON_Array) {
            printf("error config for 'Strategy' item\n");
            return -1;
        }

        cJSON* subitem = NULL;
        int index;
        int sz2 = cJSON_GetArraySize(obj);
        for (index = 0; index < sz2; ++index)
        {
            subitem = cJSON_GetArrayItem(obj, index);
            if (subitem->type != cJSON_Object) {
                continue;
            }

            zs_strategy_conf_t* pstg_conf;
            pstg_conf = (zs_strategy_conf_t*)ztl_array_at(&tradingConf->StrategyArr, index);

            cJSON* nameObj = cJSON_GetObjectItem(subitem, "name");
            cJSON* pathObj = cJSON_GetObjectItem(subitem, "path");
            cJSON* symbolObj = cJSON_GetObjectItem(subitem, "symbol");

            if (!nameObj || !pathObj) {
                continue;
            }
            strncpy(pstg_conf->Name, nameObj->valuestring, sizeof(pstg_conf->Name) - 1);

            pstg_conf->LibPath = ztl_pcalloc(pool, ztl_align(strlen(pathObj->valuestring) + 1, 8));
            strcpy(pstg_conf->LibPath, pathObj->valuestring);

            pstg_conf->Symbols = NULL;
            if (symbolObj) {
                pstg_conf->Symbols = _zs_pool_str_dup(pool, symbolObj->valuestring, -1);
            }
        }
    }


    return 0;
}

int zs_configs_load_json(zs_algo_param_t* algoParam, ztl_pool_t* pool,
    const char* confFile)
{
    int  length = 0;
    char buffer[8000] = "";
    FILE* fp = fopen(confFile, "r");
    if (!fp) {
        return -2;
    }
    length = (int)fread(buffer, sizeof(buffer) - 1, 1, fp);
    fclose(fp);

    int size, index;
    cJSON* json;
    cJSON* tnode;

    json = cJSON_Parse(buffer);
    if (!json) {
        // invalid json buffer
        return -3;
    }

    size = cJSON_GetArraySize(json);
    if (size <= 0) {
        // invalid json cofigure
        return -4;
    }

    ztl_array_init(&algoParam->TradingConf, pool, 8, sizeof(zs_trading_conf_t));

    zs_trading_conf_t* trading_conf;
    tnode = NULL;
    index = 0;
    cJSON_ArrayForEach(tnode, json)
    {
        if (tnode->type != cJSON_Object) {
            continue;
        }

        trading_conf = (zs_trading_conf_t*)ztl_array_at(&algoParam->TradingConf, index++);
        memset(trading_conf, 0, sizeof(zs_trading_conf_t));
        _zs_configs_parse_tradings(tnode, pool, trading_conf);
    }


    cJSON_Delete(json);

    return 0;
}

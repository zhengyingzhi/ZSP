#include <stdio.h>
#include <string.h>

#include <ZToolLib/ztl_config.h>
#include <ZToolLib/cJSON.h>

#include "zs_api_object.h"
#include "zs_configs.h"
#include "zs_core.h"


#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif//_MSC_VER


static char* _zs_pool_str_dup(ztl_pool_t* pool, const char* src, int length)
{
    if (length <= 0)
        length = (int)strlen(src);

    char* dst;
    dst = ztl_pcalloc(pool, ztl_align(length + 1, 8));
    memcpy(dst, src, length);
    return dst;
}

static char* _zs_read_file_content(const char* filename, int* length)
{
    int     capacity;
    char*   buffer;

    capacity = 64 * 1024;
    buffer = malloc(capacity);

    FILE* fp = fopen(filename, "r");
    if (!fp)
    {
        free(buffer);
        return NULL;
    }

    *length = (int)fread(buffer, capacity - 1, 1, fp);
    fclose(fp);

    return buffer;
}

int zs_get_string_object_value(cJSON* tnode, const char* key, char* dst, int dstsz)
{
    cJSON* obj;
    obj = cJSON_GetObjectItem(tnode, key);
    if (obj) {
        strncpy(dst, obj->valuestring, dstsz);
        return ZS_OK;
    }
    return ZS_ERROR;
}

int zs_get_int_object_value(cJSON* tnode, const char* key, int32_t* dst)
{
    cJSON* obj;
    obj = cJSON_GetObjectItem(tnode, key);
    if (obj) {
        *dst = obj->valueint;
        return ZS_OK;
    }
    return ZS_ERROR;
}

int zs_get_double_object_value(cJSON* tnode, const char* key, double* dst)
{
    cJSON* obj;
    obj = cJSON_GetObjectItem(tnode, key);
    if (obj) {
        *dst = obj->valuedouble;
        return ZS_OK;
    }
    return ZS_ERROR;
}


static int _zs_configs_parse_accounts(zs_algo_param_t* algo_params,
    cJSON* tnode, ztl_pool_t* pool,
    zs_conf_account_t* account_conf)
{
    int rv;

    rv = zs_get_string_object_value(tnode, "AccountID", account_conf->AccountID, sizeof(account_conf->AccountID) - 1);
    if (0 != rv) {
        return rv;
    }

    zs_get_string_object_value(tnode, "Password", account_conf->Password, sizeof(account_conf->Password) - 1);
    zs_get_string_object_value(tnode, "BrokerID", account_conf->BrokerID, sizeof(account_conf->BrokerID) - 1);
    zs_get_string_object_value(tnode, "TradeAPIName", account_conf->TradeAPIName, sizeof(account_conf->TradeAPIName) - 1);
    zs_get_string_object_value(tnode, "MDAPIName", account_conf->MDAPIName, sizeof(account_conf->MDAPIName) - 1);
    zs_get_string_object_value(tnode, "AppID", account_conf->AppID, sizeof(account_conf->AppID) - 1);

    account_conf->AuthCode = NULL;
    if (cJSON_HasObjectItem(tnode, "AuthCode"))
    {
        cJSON* obj;
        obj = cJSON_GetObjectItem(tnode, "AuthCode");
        account_conf->AuthCode = _zs_pool_str_dup(pool, obj->valuestring, -1);
    }

    // find the broker info
    zs_conf_broker_t* broker;
    broker = zs_configs_find_broker(algo_params, account_conf->BrokerID);
    if (!broker)
    {
        // ERRORID: not find the broker info
        return -1;
    }
    strcpy(account_conf->TradeAddr, broker->TradeAddr);
    strcpy(account_conf->MDAddr, broker->MDAddr);

    // keep the original setting
    char* account_setting = cJSON_Print(tnode);
    account_conf->AccountSetting = _zs_pool_str_dup(pool, account_setting, (int)strlen(account_setting));
    free(account_setting);

    return 0;
}


/// 加载全局配置
int zs_configs_load_global(zs_algo_param_t* algo_param, ztl_pool_t* pool,
    const char* conf_file)
{
    int length, rv;
    char*   buffer;
    cJSON*  json;
    char    log_name[256] = "";

    algo_param->RunMode = ZS_RM_Liverun;

    buffer = _zs_read_file_content(conf_file, &length);
    if (!buffer)
    {
        // ERRORID: 
        return -1;
    }

    json = cJSON_Parse(buffer);
    if (!json)
    {
        // invalid json buffer
        rv = -3;
        goto PARSE_END;
    }

    zs_get_string_object_value(json, "LogName", log_name, sizeof(log_name) - 1);
    algo_param->LogName = _zs_pool_str_dup(pool, log_name, (int)strlen(log_name));

    rv = 0;

PARSE_END:
    return rv;
}

/// 加载账户配置
int zs_configs_load_account(zs_algo_param_t* algo_param, ztl_pool_t* pool,
    const char* conf_file)
{
    int length, rv;
    int size, index;
    char*   buffer;
    cJSON*  json;
    cJSON*  tnode;

    buffer = _zs_read_file_content(conf_file, &length);
    if (!buffer)
    {
        // ERRORID: 
        return -1;
    }

    json = cJSON_Parse(buffer);
    if (!json)
    {
        // invalid json buffer
        rv = -3;
        goto PARSE_END;
    }

    size = cJSON_GetArraySize(json);
    if (size <= 0)
    {
        // invalid json cofigure
        rv = -4;
        goto PARSE_END;
    }

    zs_conf_account_t* account_conf;
    tnode = NULL;
    index = 0;
    cJSON_ArrayForEach(tnode, json)
    {
        if (tnode->type != cJSON_Object) {
            continue;
        }

        account_conf = (zs_conf_account_t*)ztl_pcalloc(pool, sizeof(zs_conf_account_t));
        if (_zs_configs_parse_accounts(algo_param, tnode, pool, account_conf) == 0) {
            ztl_array_push_back(&algo_param->AccountConf, &account_conf);
        }
    }

    cJSON_Delete(json);

    rv = 0;

PARSE_END:
    if (buffer) {
        free(buffer);
    }
    return rv;
}


/// 加载经纪商配置
int zs_configs_load_broker(zs_algo_param_t* algo_param, ztl_pool_t* pool,
    const char* conf_file)
{
    int length, rv;
    int size, index;
    char*   buffer;
    cJSON*  json;
    cJSON*  tnode;
    zs_conf_broker_t* broker_conf;

    broker_conf = (zs_conf_broker_t*)ztl_pcalloc(pool, sizeof(zs_conf_broker_t));
    strcpy(broker_conf->BrokerID, "0000");
    strcpy(broker_conf->BrokerName, "INNER");
    strcpy(broker_conf->APIName, "backtest");
    ztl_array_push_back(&algo_param->BrokerConf, &broker_conf);

#if 0
    broker_conf = (zs_conf_broker_t*)ztl_pcalloc(pool, sizeof(zs_conf_broker_t));
    strcpy(broker_conf->BrokerID, "2318");
    strcpy(broker_conf->BrokerName, "华泰仿真");
    strcpy(broker_conf->APIName, "CTP");
    strcpy(broker_conf->TradeAddr, "tcp://59.36.3.115:61206");
    strcpy(broker_conf->MDAddr, "tcp://59.36.3.115:41214");
    ztl_array_push_back(&algo_param->BrokerConf, &broker_conf);
#endif

    buffer = _zs_read_file_content(conf_file, &length);
    if (!buffer)
    {
        // ERRORID: 
        return -1;
    }

    json = cJSON_Parse(buffer);
    if (!json)
    {
        // invalid json buffer
        rv = -3;
        goto PARSE_END;
    }

    size = cJSON_GetArraySize(json);
    if (size <= 0)
    {
        // invalid json cofigure
        rv = -4;
        goto PARSE_END;
    }

    tnode = NULL;
    index = 0;
    cJSON_ArrayForEach(tnode, json)
    {
        if (tnode->type != cJSON_Object) {
            continue;
        }

        broker_conf = (zs_conf_broker_t*)ztl_pcalloc(pool, sizeof(zs_conf_broker_t));

        zs_get_string_object_value(tnode, "APIName", broker_conf->APIName, sizeof(broker_conf->APIName) - 1);
        zs_get_string_object_value(tnode, "BrokerID", broker_conf->BrokerID, sizeof(broker_conf->BrokerID) - 1);
        zs_get_string_object_value(tnode, "BrokerName", broker_conf->BrokerName, sizeof(broker_conf->BrokerName) - 1);
        zs_get_string_object_value(tnode, "TradeAddr", broker_conf->TradeAddr, sizeof(broker_conf->TradeAddr) - 1);
        zs_get_string_object_value(tnode, "MDAddr", broker_conf->MDAddr, sizeof(broker_conf->MDAddr) - 1);

        ztl_array_push_back(&algo_param->BrokerConf, &broker_conf);
    }

    cJSON_Delete(json);

    rv = 0;

PARSE_END:
    if (buffer) {
        free(buffer);
    }
    return rv;
}


/// 加载策略配置
int zs_configs_load_strategy_setting(zs_algo_param_t* algo_param, ztl_pool_t* pool,
    const char* conf_file)
{
    int length, rv;
    int size, index;
    char*   buffer;
    cJSON*  json;
    cJSON*  tnode;

    buffer = _zs_read_file_content(conf_file, &length);
    if (!buffer)
    {
        // ERRORID: 
        return -1;
    }

    json = cJSON_Parse(buffer);
    if (!json)
    {
        // invalid json buffer
        rv = -3;
        goto PARSE_END;
    }

    size = cJSON_GetArraySize(json);
    if (size <= 0)
    {
        // invalid json cofigure
        rv = -4;
        goto PARSE_END;
    }

    zs_conf_strategy_t* strategy_conf;
    tnode = NULL;
    index = 0;
    cJSON_ArrayForEach(tnode, json)
    {
        if (tnode->type != cJSON_Object) {
            continue;
        }

        strategy_conf = (zs_conf_strategy_t*)ztl_pcalloc(pool, sizeof(zs_conf_strategy_t));

        zs_get_string_object_value(tnode, "StrategyName", strategy_conf->StrategyName, sizeof(strategy_conf->StrategyName) - 1);
        zs_get_string_object_value(tnode, "Symbol", strategy_conf->Symbol, sizeof(strategy_conf->Symbol) - 1);

        // keep the original setting
        char* strategy_setting = cJSON_Print(tnode);
        strategy_conf->Setting = _zs_pool_str_dup(pool, strategy_setting, (int)strlen(strategy_setting));
        free(strategy_setting);

        ztl_array_push_back(&algo_param->StrategyConf, &strategy_conf);
    }

    cJSON_Delete(json);

    rv = 0;

PARSE_END:
    if (buffer) {
        free(buffer);
    }
    return rv;
}


/// 加载交易策略的配置
static int zs_configs_load_tradings(zs_algo_param_t* algo_param, ztl_pool_t* pool,
    const char* conf_file)
{
    int length, rv;
    int size, index;
    char*   buffer;
    cJSON*  json;
    cJSON*  tnode;

    buffer = _zs_read_file_content(conf_file, &length);
    if (!buffer)
    {
        // ERRORID: 
        return -1;
    }

    json = cJSON_Parse(buffer);
    if (!json)
    {
        // invalid json buffer
        rv = -3;
        goto PARSE_END;
    }

    size = cJSON_GetArraySize(json);
    if (size <= 0)
    {
        // invalid json cofigure
        rv = -4;
        goto PARSE_END;
    }

    zs_conf_trading_t* trading_conf;
    tnode = NULL;
    index = 0;
    cJSON_ArrayForEach(tnode, json)
    {
        if (tnode->type != cJSON_Object) {
            continue;
        }

        trading_conf = (zs_conf_trading_t*)ztl_pcalloc(pool, sizeof(zs_conf_trading_t));

        rv = zs_get_string_object_value(tnode, "AccountID", trading_conf->AccountID, sizeof(trading_conf->AccountID) - 1);
        if (0 != rv) {
            continue;
        }

        rv = zs_get_string_object_value(tnode, "StrategyName", trading_conf->StrategyName, sizeof(trading_conf->StrategyName) - 1);
        if (0 != rv) {
            continue;
        }

        zs_get_string_object_value(tnode, "Symbol", trading_conf->Symbol, sizeof(trading_conf->Symbol) - 1);

        // keep the original setting
        char* trading_setting = cJSON_Print(tnode);
        trading_conf->Setting = _zs_pool_str_dup(pool, trading_setting, (int)strlen(trading_setting));
        free(trading_setting);

        ztl_array_push_back(&algo_param->TradingConf, &trading_conf);
    }

    cJSON_Delete(json);

    rv = 0;

PARSE_END:
    if (buffer) {
        free(buffer);
    }
    return rv;
}


/// 初始化参数
int zs_algo_param_init(zs_algo_param_t* algo_param)
{
    ztl_array_init(&algo_param->BrokerConf, NULL, 16, sizeof(zs_conf_broker_t));
    ztl_array_init(&algo_param->AccountConf, NULL, 8, sizeof(zs_conf_account_t));
    ztl_array_init(&algo_param->StrategyConf, NULL, 16, sizeof(zs_conf_strategy_t));
    ztl_array_init(&algo_param->TradingConf, NULL, 16, sizeof(zs_conf_trading_t));

    return ZS_OK;
}


/// 加载配置入口
int zs_configs_load(zs_algo_param_t* algo_param, ztl_pool_t* pool)
{
    int rv;
    const char* zs_conf_file = "zs_config.json";
    rv = zs_configs_load_global(algo_param, pool, zs_conf_file);

    const char* zs_broker_conf_file = "zs_brokers.json";
    rv = zs_configs_load_broker(algo_param, pool, zs_broker_conf_file);

    const char* zs_account_conf_file = "zs_account.json";
    rv = zs_configs_load_account(algo_param, pool, zs_account_conf_file);

    const char* zs_strategy_conf_file = "zs_strategy.json";
    rv = zs_configs_load_strategy_setting(algo_param, pool, zs_strategy_conf_file);

    const char* zs_tradings_conf_file = "zs_tradings.json";
    rv = zs_configs_load_tradings(algo_param, pool, zs_tradings_conf_file);

    return rv;
}


zs_conf_broker_t* zs_configs_find_broker(zs_algo_param_t* algo_param, const char* brokerid)
{
    zs_conf_broker_t* broker_conf;
    uint32_t count;
    count = ztl_array_size(&algo_param->BrokerConf);

    for (uint32_t i = 0; i < count; ++i)
    {
        broker_conf = (zs_conf_broker_t*)ztl_array_at2(&algo_param->BrokerConf, i);
        if (strcasecmp(broker_conf->BrokerID, brokerid) == 0)
            return broker_conf;
    }

    return NULL;
}

zs_conf_account_t* zs_configs_find_account(zs_algo_param_t* algo_param, const char* accountid)
{
    zs_conf_account_t* account_conf;
    uint32_t count;
    count = ztl_array_size(&algo_param->AccountConf);

    for (uint32_t i = 0; i < count; ++i)
    {
        account_conf = (zs_conf_account_t*)ztl_array_at2(&algo_param->AccountConf, i);
        if (strcmp(account_conf->AccountID, accountid) == 0)
            return account_conf;
    }

    return NULL;
}

zs_conf_strategy_t* zs_configs_find_strategy(zs_algo_param_t* algo_param, const char* strategy_name)
{
    zs_conf_strategy_t* strategy_conf;
    uint32_t count;
    count = ztl_array_size(&algo_param->StrategyConf);

    for (uint32_t i = 0; i < count; ++i)
    {
        strategy_conf = (zs_conf_strategy_t*)ztl_array_at2(&algo_param->StrategyConf, i);
        if (strcmp(strategy_conf->StrategyName, strategy_name) == 0)
            return strategy_conf;
    }

    return NULL;
}

//////////////////////////////////////////////////////////////////////////

struct zs_json_s
{
    cJSON*  cjson;
};

zs_json_t* zs_json_parse(const char* buffer, int32_t length)
{
    (void)length;

    cJSON* cjson;
    cjson = cJSON_Parse(buffer);
    if (!cjson)
    {
        // invalid json buffer
        return NULL;
    }

    zs_json_t* zjson = (zs_json_t*)malloc(sizeof(zs_json_t));
    zjson->cjson = cjson;
    return zjson;
}

void zs_json_release(zs_json_t* zjson)
{
    if (zjson)
    {
        if (zjson->cjson)
            cJSON_Delete(zjson->cjson);
        free(zjson);
    }
}

int zs_json_have_object(zs_json_t* zjson, const char* key)
{
    if (zjson) {
        return cJSON_HasObjectItem(zjson->cjson, key);
    }
    return 0;
}

int zs_json_get_object(zs_json_t* zjson, const char* key, zs_json_t** pvalue)
{
    return -1;
}

int zs_json_get_string(zs_json_t* zjson, const char* key, char value[], int size)
{
    cJSON* obj;
    obj = cJSON_GetObjectItem(zjson->cjson, key);
    if (obj) {
        strncpy(value, obj->valuestring, size);
        return 0;
    }
    return -1;
}

int zs_json_get_int(zs_json_t* zjson, const char* key, int* value)
{
    cJSON* obj;
    obj = cJSON_GetObjectItem(zjson->cjson, key);
    if (obj) {
        *value = obj->valueint;
        return 0;
    }
    return -1;
}

int zs_json_get_double(zs_json_t* zjson, const char* key, double* value)
{
    cJSON* obj;
    obj = cJSON_GetObjectItem(zjson->cjson, key);
    if (obj) {
        *value = obj->valuedouble;
        return 0;
    }
    return -1;
}

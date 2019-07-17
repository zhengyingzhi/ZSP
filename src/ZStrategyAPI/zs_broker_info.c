#include <ZToolLib/cJSON.h>
#include <ZToolLib/ztl_utils.h>
#include <ZToolLib/ztl_config.h>

#include "zs_broker_info.h"

#ifdef _MSC_VER
#define zs_strdup   _strdup
#else
#define zs_strdup   strdup
#endif//_MSC_VER

static int _zs_get_string_object_value(cJSON* tnode, const char* key, char* dst, int dstsz)
{
    cJSON* obj;
    obj = cJSON_GetObjectItem(tnode, key);
    if (obj) {
        strncpy(dst, obj->valuestring, dstsz);
        return 0;
    }
    return -1;
}

int zs_broker_info_load(ztl_vector_t* broker_infos, const char* setting_file)
{
    int     rv;
    int     length;
    char*   buffer;

    rv = 0;
    length = 16 * 1024;
    buffer = (char*)malloc(length);
    memset(buffer, 0, length);

    length = read_file_content(setting_file, buffer, length - 1);
    if (length != 0) {
        // ERRORID: read file failed
        rv = -3;
        goto LOAD_END;
    }

    int size, index;
    cJSON* json;
    cJSON* tnode;

    json = cJSON_Parse(buffer);
    if (!json) {
        // invalid json buffer
        rv = -3;
        goto LOAD_END;
    }

    size = cJSON_GetArraySize(json);
    if (size <= 0) {
        // invalid json cofigure
        rv = -4;
        goto LOAD_END;
    }

    zs_broker_info_t* broker_info = NULL;
    tnode = NULL;
    index = 0;
    cJSON_ArrayForEach(tnode, json)
    {
        if (tnode->type != cJSON_Object) {
            continue;
        }

        if (!broker_info)
            broker_info = calloc(1, sizeof(zs_broker_info_t));

        _zs_get_string_object_value(tnode, "BrokerName", broker_info->BrokerName, sizeof(broker_info->BrokerName) - 1);
        _zs_get_string_object_value(tnode, "BrokerID", broker_info->BrokerID, sizeof(broker_info->BrokerID) - 1);
        if (!broker_info->BrokerName[0] || !broker_info->BrokerID[0]) {
            continue;
        }

        cJSON* AddrJson;
        AddrJson = cJSON_GetObjectItem(tnode, "TradeAddr");
        if (!AddrJson) {
            continue;
        }
        size = cJSON_GetArraySize(AddrJson);
        if (size == 0) {
            continue;
        }

        index = 0;
        cJSON* subnode = NULL;
        cJSON_ArrayForEach(subnode, AddrJson)
        {
            if (index == ZS_MAX_ADDR_NUM) {
                break;
            }

            char* addr = zs_strdup(subnode->valuestring);
            broker_info->TradeAddr[index++] = addr;
        }

        AddrJson = cJSON_GetObjectItem(tnode, "MdAddr");
        if (!AddrJson) {
            continue;
        }
        size = cJSON_GetArraySize(AddrJson);
        if (size == 0) {
            continue;
        }

        index = 0;
        subnode = NULL;
        cJSON_ArrayForEach(subnode, AddrJson)
        {
            if (index == ZS_MAX_ADDR_NUM) {
                break;
            }

            char* addr = zs_strdup(subnode->valuestring);
            broker_info->MdAddr[index++] = addr;
        }

        broker_infos->push_x(broker_infos, broker_info);

        broker_info = NULL;
    }

    cJSON_Delete(json);

LOAD_END:
    if (buffer) {
        free(buffer);
    }
    return rv;
}

zs_broker_info_t* zs_broker_find_byid(ztl_vector_t* broker_infos, const char* broker_id)
{
    zs_broker_info_t* broker_info;
    zs_broker_info_t* array = (zs_broker_info_t*)broker_infos->elts;
    for (uint32_t i = 0; i < broker_infos->nelts; ++i)
    {
        broker_info = array + i;
        if (strcmp(broker_info->BrokerID, broker_id) == 0) {
            return broker_info;
        }
    }
    return NULL;
}

zs_broker_info_t* zs_broker_find_byname(ztl_vector_t* broker_infos, const char* broker_name)
{
    zs_broker_info_t* broker_info;
    zs_broker_info_t* array = (zs_broker_info_t*)broker_infos->elts;
    for (uint32_t i = 0; i < broker_infos->nelts; ++i)
    {
        broker_info = array + i;
        if (strcmp(broker_info->BrokerName, broker_name) == 0) {
            return broker_info;
        }
    }
    return NULL;
}


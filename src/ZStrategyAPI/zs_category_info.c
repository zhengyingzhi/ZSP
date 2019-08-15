#include <assert.h>
#include <ctype.h>

#include <ZToolLib/cJSON.h>
#include <ZToolLib/ztl_utils.h>
#include <ZToolLib/ztl_config.h>

#include "zs_category_info.h"
#include "zs_constants_helper.h"
#include "zs_error.h"

#ifdef _MSC_VER
#define zs_strdup   _strdup
#else
#define zs_strdup   strdup
#endif//_MSC_VER


typedef union
{
    char     c[4];
    uint32_t i;
}ZSVarietyInt;

uint32_t zs_get_variety_int(const char* symbol)
{
#ifdef _DEBUG
    assert(sizeof(ZSVarietyInt) == 4);
#endif

    ZSVarietyInt vi;
    vi.i = 0;

    if (isdigit(symbol[0]))
    {
        vi.i = atoi(symbol);
    }
    else
    {
        vi.c[0] = symbol[0];
        if (symbol[1])
        {
            if (isalpha(symbol[1]))
                vi.c[1] = symbol[1];
            if (symbol[2] && isalpha(symbol[2]))
                vi.c[2] = symbol[2];
        }
    }

    return vi.i;
}

extern int zs_get_string_object_value(cJSON* tnode, const char* key, char* dst, int dstsz);
extern int zs_get_int_object_value(cJSON* tnode, const char* key, int32_t* dst);
extern int zs_get_double_object_value(cJSON* tnode, const char* key, double* dst);


int zs_category_init(zs_category_t* category_obj)
{
    category_obj->Pool  = ztl_create_pool(10 * 1024);
    category_obj->Dict  = dictCreate(&uintHashDictType, category_obj);
    category_obj->Count = 0;
    return ZS_OK;
}

int zs_category_release(zs_category_t* category_obj)
{
    if (category_obj->Pool)
    {
        dictRelease(category_obj->Dict);
        ztl_destroy_pool(category_obj->Pool);
        category_obj->Dict = NULL;
        category_obj->Pool = NULL;
        category_obj->Count = 0;
    }
    return ZS_OK;
}

int zs_category_load(zs_category_t* category_obj, const char* info_file)
{
    int     rv;
    int     length;
    char*   buffer;

    rv = 0;
    length = 64 * 1024;
    buffer = (char*)malloc(length);
    memset(buffer, 0, length);

    length = read_file_content(info_file, buffer, length - 1);
    if (length != 0) {
        // ERRORID: read file failed
        rv = ZS_ERR_NoMemory;
        goto LOAD_END;
    }

    int size, index;
    cJSON* json;
    cJSON* tnode;

    json = cJSON_Parse(buffer);
    if (!json) {
        // invalid json buffer
        rv = ZS_ERR_JsonData;
        goto LOAD_END;
    }

    size = cJSON_GetArraySize(json);
    if (size <= 0) {
        // invalid json cofigure
        rv = ZS_ERR_JsonData;
        goto LOAD_END;
    }

    uint64_t variety_int = 0;
    uint32_t alloc_size = ztl_align(sizeof(zs_category_info_t), 8);
    zs_category_info_t* ci = NULL;
    tnode = NULL;
    index = 0;
    cJSON_ArrayForEach(tnode, json)
    {
        if (tnode->type != cJSON_Object) {
            continue;
        }

        if (!ci)
            ci = ztl_pcalloc(category_obj->Pool, alloc_size);

        zs_get_string_object_value(tnode, "product",    ci->Product, sizeof(ci->Product) - 1);
        zs_get_string_object_value(tnode, "code",       ci->Code, sizeof(ci->Code) - 1);
        zs_get_string_object_value(tnode, "exchange",   ci->Exchange, sizeof(ci->Exchange) - 1);
        zs_get_string_object_value(tnode, "name",       ci->Name, sizeof(ci->Name) - 1);
        zs_get_string_object_value(tnode, "last_trading_day", ci->LastTradingDay, sizeof(ci->LastTradingDay) - 1);
        if (!ci->Code[0]) {
            continue;
        }

        zs_get_double_object_value(tnode, "price_tick", &ci->PriceTick);
        zs_get_int_object_value(tnode, "multiplier", &ci->Multiplier);
        zs_get_int_object_value(tnode, "decimal_digit", &ci->Decimal);

        int32_t comm_type = 0;
        zs_get_int_object_value(tnode, "commission_type", &comm_type);
        ci->CommType = (comm_type == 1) ? ZS_COMMT_ByVolume : ZS_COMMT_ByMoney;
        zs_get_double_object_value(tnode, "commission_open", &ci->OpenRatio);
        zs_get_double_object_value(tnode, "commission_yesterday", &ci->CloseRatio);
        zs_get_double_object_value(tnode, "commission_today", &ci->CloseTodayRatio);
        zs_get_double_object_value(tnode, "margin_rate", &ci->MarginRatio);

        variety_int = zs_get_variety_int(ci->Code);
        dictAdd(category_obj->Dict, (void*)variety_int, ci);

        ci = NULL;
    }

    cJSON_Delete(json);

LOAD_END:
    if (buffer) {
        free(buffer);
    }
    return rv;
}

zs_category_info_t* zs_category_find(zs_category_t* category_obj, const char* code)
{
    dictEntry*  entry;
    uint64_t    variety_int;

    variety_int = zs_get_variety_int(code);
    entry = dictFind(category_obj->Dict, (void*)variety_int);
    if (entry) {
        return (zs_category_info_t*)entry->v.val;
    }

    return NULL;
}


void zs_category_to_contract(zs_contract_t* contract, const char* symbol, const zs_category_info_t* category_info)
{
    contract->ExchangeID = zs_convert_exchange_name(category_info->Exchange, 0);
    strncpy(contract->Symbol, symbol, sizeof(contract->Symbol) - 1);
    strncpy(contract->SymbolName, category_info->Name, sizeof(contract->SymbolName) - 1);
    contract->PriceTick     = category_info->PriceTick;
    contract->Multiplier    = category_info->Multiplier;
    contract->Decimal       = category_info->Decimal;
    contract->LongMarginRateByMoney     = category_info->MarginRatio;
    contract->ShortMarginRateByMoney    = category_info->MarginRatio;

    if (category_info->CommType == ZS_COMMT_ByMoney)
    {
        contract->OpenRatioByMoney = category_info->OpenRatio;
        contract->CloseRatioByMoney = category_info->CloseRatio;
        contract->CloseTodayRatioByMoney = category_info->CloseTodayRatio;
    }
    else
    {
        contract->OpenRatioByVolume = category_info->OpenRatio;
        contract->CloseRatioByVolume = category_info->CloseRatio;
        contract->CloseTodayRatioByVolume = category_info->CloseTodayRatio;
    }

    // FIXME
    if (strcmp(category_info->Product, "future") == 0)
        contract->ProductClass = ZS_PC_Future;
    else
        contract->ProductClass = ZS_PC_Stock;
}

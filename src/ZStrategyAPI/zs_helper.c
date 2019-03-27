
#include <ZToolLib/ztl_atomic.h>

#ifdef _MSC_VER
#include <Windows.h>
#else
#endif

#include "zs_common.h"

#include "zs_core.h"


uint32_t zs_data_increment(zs_data_head_t* zdh)
{
    return ztl_atomic_add(&zdh->RefCount, 1);
}

uint32_t zs_data_decre_release(zs_data_head_t* zdh)
{
    uint32_t oldv = ztl_atomic_dec(&zdh->RefCount, 1);
    if (1 == oldv)
    {
        if (zdh->Cleanup)
            zdh->Cleanup(zdh);
    }
    return oldv;
}


ZSExchangeID zs_convert_exchange_name(const char* exchange_name)
{
    if (strcmp(exchange_name, ZS_EXG_SHFE) == 0) {
        return ZS_EI_SHFE;
    }
    else if (strcmp(exchange_name, ZS_EXG_DCE) == 0) {
        return ZS_EI_DCE;
    }
    else if (strcmp(exchange_name, ZS_EXG_CZCE) == 0) {
        return ZS_EI_CZCE;
    }
    else if (strcmp(exchange_name, ZS_EXG_INE) == 0) {
        return ZS_EI_INE;
    }
    else if (strcmp(exchange_name, ZS_EXG_CFFEX) == 0) {
        return ZS_EI_CFFEX;
    }

    return ZS_EI_Unkown;
}

const char* zs_convert_exchange_id(ZSExchangeID exchange_id)
{
    static const char* exchange_names[] = { "UNKNOWN", ZS_EXG_SSE, ZS_EXG_SZSE, ZS_EXG_NEEQ,
        ZS_EXG_CFFEX, ZS_EXG_SHFE, ZS_EXG_DCE, ZS_EXG_CZCE, ZS_EXG_INE, ZS_EXG_SGE, ZS_EXG_CFE, ZS_EXG_TEST };
    return exchange_names[exchange_id];
}


#include <ZToolLib/ztl_atomic.h>

#ifdef _MSC_VER

#include <Windows.h>

#else

#endif

#include "zs_constants_helper.h"

#include "zs_core.h"


const char* exchange_names[] = { ZS_EXCHANGE_UNKNOWN,
    ZS_EXCHANGE_SSE, ZS_EXCHANGE_SZSE, ZS_EXCHANGE_NEEQ,
    ZS_EXCHANGE_CFFEX, ZS_EXCHANGE_SHFE, ZS_EXCHANGE_DCE, ZS_EXCHANGE_CZCE, ZS_EXCHANGE_INE,
    ZS_EXCHANGE_SGE, ZS_EXCHANGE_CFE, ZS_EXCHANGE_TEST
};

int finished_status_table[] = { 0, 0, 0, 0, 1, 1, 1, 1, 0 };


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
    if (strcmp(exchange_name, ZS_EXCHANGE_SSE) == 0) {
        return ZS_EI_SSE;
    }
    else if (strcmp(exchange_name, ZS_EXCHANGE_SZSE) == 0) {
        return ZS_EI_SZSE;
    }
    else if (strcmp(exchange_name, ZS_EXCHANGE_SHFE) == 0) {
        return ZS_EI_SHFE;
    }
    else if (strcmp(exchange_name, ZS_EXCHANGE_DCE) == 0) {
        return ZS_EI_DCE;
    }
    else if (strcmp(exchange_name, ZS_EXCHANGE_CZCE) == 0) {
        return ZS_EI_CZCE;
    }
    else if (strcmp(exchange_name, ZS_EXCHANGE_INE) == 0) {
        return ZS_EI_INE;
    }
    else if (strcmp(exchange_name, ZS_EXCHANGE_CFFEX) == 0) {
        return ZS_EI_CFFEX;
    }

    return ZS_EI_Unkown;
}

const char* zs_convert_exchange_id(ZSExchangeID exchange_id)
{
    return exchange_names[exchange_id];
}

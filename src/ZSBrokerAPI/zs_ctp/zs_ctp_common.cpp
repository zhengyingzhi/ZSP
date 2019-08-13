#include "zs_ctp_common.h"



static const char* exchange_names[] = { ZS_EXCHANGE_UNKNOWN,
    ZS_EXCHANGE_SSE, ZS_EXCHANGE_SZSE, ZS_EXCHANGE_NEEQ,
    ZS_EXCHANGE_CFFEX, ZS_EXCHANGE_SHFE, ZS_EXCHANGE_DCE, ZS_EXCHANGE_CZCE, ZS_EXCHANGE_INE,
    ZS_EXCHANGE_SGE, ZS_EXCHANGE_CFE, ZS_EXCHANGE_TEST
};


const char* get_exchange_name(ZSExchangeID exchangeid)
{
    return exchange_names[exchangeid];
}

ZSExchangeID get_exchange_id(const char* exchange_name)
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

void conv_rsp_info(zs_error_data_t* error, CThostFtdcRspInfoField *pRspInfo)
{
    if (pRspInfo) {
        strcpy(error->ErrorMsg, pRspInfo->ErrorMsg);
        error->ErrorID = pRspInfo->ErrorID;
    }
    else {
        error->ErrorMsg[0] = '\0';
        error->ErrorID = 0;
    }
}

int32_t conv_ctp_time(const char* stime)
{
    int32_t t;
    t = atoi(stime) * 10000 + atoi(stime + 3) * 100 + atoi(stime + 6);
    return t;
}

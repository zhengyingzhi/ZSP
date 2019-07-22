#include "zs_ctp_common.h"



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
    t = atoi(stime) * 10000 + atoi(stime + 2) * 100 + atoi(stime + 4);
    return t;
}

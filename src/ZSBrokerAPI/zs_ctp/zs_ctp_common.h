#ifndef _ZS_CTP_COMMON_H_
#define _ZS_CTP_COMMON_H_


#include <ZStrategyAPI/zs_api_object.h>
#include <ZStrategyAPI/zs_core.h>
#include <ZStrategyAPI/zs_configs.h>

#undef ZS_HAVE_SE
#ifdef ZS_HAVE_SE
#include <ctp_se/ThostFtdcUserApiStruct.h>
#else
#include <ctp/ThostFtdcUserApiStruct.h>
#endif//ZS_HAVE_SE

#ifdef _MSC_VER

#ifdef ZS_CTP_EXPORTS
#define ZS_CTP_API __declspec(dllexport)
#else
#define ZS_CTP_API __declspec(dllimport)
#endif

#else/* linux */
#define ZS_CTP_API 
#endif//_MSC_VER

#ifdef __cplusplus
extern "C" {
#endif


const char* get_exchange_name(ZSExchangeID exchangeid);

ZSExchangeID get_exchange_id(const char* exchange_name);


void conv_rsp_info(zs_error_data_t* error, CThostFtdcRspInfoField *pRspInfo);

int32_t conv_ctp_time(const char* stime);


#ifdef __cplusplus
}
#endif

#endif//_ZS_CTP_COMMON_H_

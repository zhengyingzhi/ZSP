#ifndef _ZS_CTP_COMMON_H_
#define _ZS_CTP_COMMON_H_


#include <ZStrategyAPI/zs_api_object.h>
#include <ZStrategyAPI/zs_core.h>

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

/* trader apis */
ZS_CTP_API void* trade_create(const char* str, int reserve);

ZS_CTP_API void trade_release(void* apiInstance);

ZS_CTP_API void trade_regist(void* apiInstance, zs_trade_api_handlers_t* handlers,
    void* tdCtx, const zs_broker_conf_t* apiConf);

ZS_CTP_API void trade_connect(void* apiInstance, void* addr);

ZS_CTP_API int trade_order(void* apiInstance, const zs_order_t* orderReq);

ZS_CTP_API int trade_cancel(void* apiInstance, const zs_cancel_req_t* cancelReq);



/* md apis */
ZS_CTP_API void* md_create(const char* str, int reserve);

ZS_CTP_API void md_release(void* apiInstance);

ZS_CTP_API void md_regist(void* apiInstance, zs_md_api_handlers_t* handlers,
    void* tdCtx, const zs_broker_conf_t* apiConf);

ZS_CTP_API void md_connect(void* apiInstance, void* addr);

ZS_CTP_API int md_login(void* apiInstance);

ZS_CTP_API int md_subscribe(void* apiInstance, char* ppInstruments[], int count);

ZS_CTP_API int md_unsubscribe(void* apiInstance, char* ppInstruments[], int count);


#ifdef __cplusplus
}
#endif

#endif//_ZS_CTP_COMMON_H_

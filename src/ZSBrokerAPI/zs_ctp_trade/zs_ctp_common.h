#ifndef _ZS_CTP_COMMON_H_
#define _ZS_CTP_COMMON_H_


#include <ZStrategyAPI/zs_api_object.h>
#include <ZStrategyAPI/zs_core.h>
#include <ZStrategyAPI/zs_configs.h>

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
void* trade_create(const char* str, int reserve);

void trade_release(void* instance);

void trade_regist(void* instance, zs_trade_api_handlers_t* handlers,
    void* tdctx, const zs_conf_broker_t* conf);

void trade_connect(void* instance, void* addr);

int trade_order(void* instance, const zs_order_t* order_req);

int trade_cancel(void* instance, const zs_cancel_req_t* cancel_req);



/* md apis */
void* md_create(const char* str, int reserve);

void md_release(void* instance);

void md_regist(void* instance, zs_md_api_handlers_t* handlers,
    void* mdctx, const zs_conf_broker_t* conf);

void md_connect(void* instance, void* addr);

int md_login(void* instance);

int md_subscribe(void* instance, char* ppInstruments[], int count);

int md_unsubscribe(void* instance, char* ppInstruments[], int count);


/* the exported dso entry
 */
ZS_CTP_API int trade_api_entry(zs_trade_api_t* tdapi);


#ifdef __cplusplus
}
#endif

#endif//_ZS_CTP_COMMON_H_

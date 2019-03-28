/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the broker api
 */

#ifndef _ZS_BROKER_API_H_
#define _ZS_BROKER_API_H_

#include <stdint.h>

#include "zs_api_object.h"

#include "zs_configs.h"

#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif



/* 对于各API库，需要提供标准接口名字
 */
static const char* zs_trade_api_names[] = { "trade_create", "trade_release", "trade_regist", \
    "trade_connect", "trade_login", "trade_order", "trade_quote_order", "trade_cancel", "trade_query", NULL };
static const char* zs_md_api_names[] = { "md_create", "md_release", "md_regist", \
    "md_connect", "md_login", "md_subscribe", "md_unsubscribe", NULL };

//struct zs_api_name_type_s
//{
//    const char* api_name;
//    void*       api_func;
//};

typedef enum
{
    ZS_API_QryAccount,
    ZS_API_QryInvestor,
    ZS_API_QryOrder,
    ZS_API_QryQuoteOrder,
    ZS_API_QryTrade,
    ZS_API_QryQuoteTrade,
    ZS_API_QryPosition,
    ZS_API_QryPositionDetail,
    ZS_API_QryContract,
    ZS_API_QryMarginRate,
    ZS_API_QryCommissionRate,
    ZS_API_QrySettlement,
    ZS_API_QryForQuote,
    ZS_API_QryMarketData
}ZSApiQueryCategory;

typedef enum
{
    ZS_API_RtnOrder,
    ZS_API_RtnQuoteOrder,
    ZS_API_RtnTrade,
    ZS_API_RtnQuoteTrade,
    ZS_API_RtnPosition,
    ZS_API_RtnFundAccount,
    ZS_API_RtnForQuote,
    ZS_API_RtnMarketData,
    ZS_API_RtnExchangeState
}ZSApiRtnCategory;

/* trader apis */
struct zs_trade_api_s
{
    const char* ApiName;
    void* ApiInstance;
    void* UserData;

    // create api instance
    void* (*create)(const char* str, int reserve);

    // release td api instance
    void (*release)(void* api_instance);

    int (*regist)(void* api_instance, zs_trade_api_handlers_t* handlers, 
        void* tdctx, const zs_broker_conf_t* conf);

    // connect
    int (*connect)(void* api_instance, void* addr);

    // login
    int (*login)(void* api_instance);

    // logout
    int (*logout)(void* api_instance);

    // order request
    int (*order)(void* api_instance, zs_order_req_t* orderReq);

    // quote order request
    int (*quote_order)(void* api_instance, zs_quote_order_req_t* quote_req);

    // order cancel request
    int (*cancel)(void* api_instance, zs_cancel_req_t* cancel_req);

    // query api
    int (*query)(void* api_instance, ZSApiQueryCategory category, void* data, int size);

    // other request api
    int (*request_other)(void* api_instance, int dtype, void* data, int size);
};

struct zs_trade_api_handlers_s
{
    void (*on_connect)(void* tdctx);
    void (*on_disconnect)(void* tdctx, int reason);
    void (*on_login)(void* tdctx, zs_login_t* login_rsp, zs_error_data_t* errdata);
    void (*on_logout)(void* tdctx, zs_logout_t* login_out, zs_error_data_t* errdata);
    void (*on_rtn_order)(void* tdctx, zs_order_t* order);
    void (*on_rtn_trade)(void* tdctx, zs_trade_t* trade);
    void (*on_rtn_data)(void* tdctx, int dtype, void* data, int size);
    void (*on_rsp_data)(void* tdctx, int dtype, void* data, int size, zs_error_data_t* errdata, uint32_t flag);
};


/* md apis */
struct zs_md_api_s
{
    const char* ApiName;
    void* ApiInstance;
    void* UserData;

    // create md api instance
    void* (*create)(const char* str, int reserve);

    // release api instance
    void (*release)(void* api_instance);

    int (*regist)(void* api_instance, zs_md_api_handlers_t* handlers,
        void* mdctx, const zs_broker_conf_t* conf);

    // connect
    int (*connect)(void* api_instance, void* addr);
    
    // login
    int (*login)(void* api_instance);

    // logout
    int (*logout)(void* api_instance);

    // subsribe market data
    int (*subscribe)(void* api_instance, zs_subscribe_t* sub_reqs[], int count);

    // unsubscribe market data
    int (*unsubscribe)(void* api_instance, zs_subscribe_t* unsub_reqs[], int count);

    // other request api
    int (*request_other)(void* api_instance, int dtype, void* data, int size);
};

struct zs_md_api_handlers_s
{
    void (*on_connect)(void* mdctx);
    void (*on_disconnect)(void* mdctx, int reason);
    void (*on_login)(void* mdctx, zs_login_t* login_rsp, zs_error_data_t* errdata);
    void (*on_logout)(void* mdctx, zs_logout_t* logout_rsp, zs_error_data_t* errdata);
    void (*on_subscribe)(void* mdctx, zs_subscribe_t* sub_rsp, int flag);
    void (*on_unsubscribe)(void* mdctx, zs_subscribe_t* unsub_rsp, int flag);
    void (*on_rtn_mktdata)(void* mdctx, zs_tick_t* tick);
    void (*on_rtn_mktdata_l2)(void* mdctx, zs_l2_tick_t* tickl2);
    void (*on_rtn_forquote)(void* mdctx, void* forQuote);
};


#ifdef __cplusplus
}
#endif

#endif//_ZS_BROKER_API_H_


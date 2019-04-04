/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the broker api
 */

#ifndef _ZS_EVENT_ENGINE_H_
#define _ZS_EVENT_ENGINE_H_

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_palloc.h>
#include <ZToolLib/ztl_producer_consumer.h>

#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif

#define ZS_EVENT_ENGINE_MAX_QUEUED  65536
#define ZS_EVENT_ENGINE_MAX_EVID    64

typedef enum
{
    ZS_EV_Timer,
    ZS_EV_Connect,
    ZS_EV_Order,
    ZS_EV_Trade,
    ZS_EV_Position,
    ZS_EV_PositionDetail,
    ZS_EV_Account,
    ZS_EV_Investor,
    ZS_EV_Contract,
    ZS_EV_Tick,
    ZS_EV_Tickl2,
    ZS_EV_Bar,
    ZS_EV_Other
}ZSEventCategory;


typedef void (*zs_ee_handler_pt)(zs_event_engine_t* ee, void* userData, uint32_t evtype, void* evdata);
struct zs_evobject_s
{
    zs_ee_handler_pt    EvHandler;
    void*               EvUserData;
};
typedef struct zs_evobject_s zs_evobject_t;


typedef ztl_producer_consumer_t ztl_pccore_t;

/* event engine */
struct zs_event_engine_s
{
    ztl_pool_t*         Pool;
    ztl_pccore_t*       ZPCCore;

    // index is evtype, elem is ztl_array_t*<ev_object_t>
    ztl_array_t*        EvObjectTable[ZS_EVENT_ENGINE_MAX_EVID];
};

zs_event_engine_t* zs_ee_create(ztl_pool_t* pool, ZSRunMode run_mode);
void zs_ee_release(zs_event_engine_t* ee);

void zs_ee_start(zs_event_engine_t* ee);
void zs_ee_stop(zs_event_engine_t* ee);

/* register event handler
 * we could register multiple handlers for a same evtype
 * evtype: ZSEventCategory, which must be < ZS_EVENT_ENGINE_MAX_EVID
 */
int zs_ee_register(zs_event_engine_t* ee, void* userdata, uint32_t evtype, zs_ee_handler_pt handler);

/* post event data to handle by consumer thread
 * evtype usually is ZSEventCategory, and evdata usually is zs
 */
int zs_ee_post(zs_event_engine_t* ee, uint32_t evtype, void* evdata);


/* directly do callback by the handler which registered before,
 * attention use, this is usually used in consumer thread
 */
int zs_ee_do_callback(zs_event_engine_t* ee, uint32_t evtype, void* evdata);


#ifdef __cplusplus
}
#endif

#endif//_ZS_EVENT_ENGINE_H_



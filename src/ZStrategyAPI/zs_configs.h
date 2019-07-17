/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the global algorithm configuration
 */

#ifndef _ZS_CONFIGS_H_INCLUDED_
#define _ZS_CONFIGS_H_INCLUDED_

#include <stdint.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_palloc.h>
#include <ZToolLib/ztl_producer_consumer.h>

#include "zs_api_object.h"
#include "zs_core.h"


#ifdef __cplusplus
extern "C" {
#endif


#define ZS_APINAME_SIZE         16
#define ZS_ACCOUNTID_SIZE       16
#define ZS_PASSWORD_SIZE        16
#define ZS_BROKERID_SIZE        8
#define ZS_BROKERNAME_SIZE      16
#define ZS_BROKER_ADDR_SIZE     64
#define ZS_STRATEGYNAME_SIZE    16
#define ZS_APPID_SIZE           32


/* ��������Ϣ����
 */
struct zs_conf_broker_s
{
    char        ApiName[ZS_APINAME_SIZE];
    char        BrokerID[ZS_BROKERID_SIZE];
    char        BrokerName[ZS_BROKERNAME_SIZE];
    char        TradeAddr[ZS_BROKER_ADDR_SIZE];
    char        MDAddr[ZS_BROKER_ADDR_SIZE];
};
typedef struct zs_conf_broker_s zs_conf_broker_t;


/* �˻�����
 */
struct zs_conf_account_s
{
    char        AccountID[ZS_ACCOUNTID_SIZE];
    char        AccountName[ZS_ACCOUNTID_SIZE];
    char        Password[ZS_PASSWORD_SIZE];
    char        BrokerID[ZS_BROKERID_SIZE];
    char        TradeAddr[ZS_BROKER_ADDR_SIZE];     // retrieve from broker setting
    char        MDAddr[ZS_BROKER_ADDR_SIZE];        // retrieve from broker setting
    char        TradeApiName[ZS_APINAME_SIZE];
    char        MDApiName[ZS_APINAME_SIZE];
    char        AppID[32];
    char*       AuthCode;
    char*       AccountSetting;
};
typedef struct zs_conf_account_s zs_conf_account_t;


/* ������Ϣ����
 */
struct zs_conf_strategy_s
{
    char        StrategyName[ZS_STRATEGYNAME_SIZE];
    char        Symbol[32];
    char*       Setting;        // ����������
};
typedef struct zs_conf_strategy_s zs_conf_strategy_t;


/* �Զ����׵Ĳ�������
 */
struct zs_conf_trading_s
{
    char        AccountID[ZS_ACCOUNTID_SIZE];
    char        StrategyName[ZS_STRATEGYNAME_SIZE];
    char        Symbol[32];
    char*       Setting;        // ����������
};
typedef struct zs_conf_trading_s zs_conf_trading_t;



/* ȫ�ֽ��ײ���
 */
struct zs_algo_param_s
{
    // global params
    ZSRunMode       RunMode;
    char*           LogName;
    int             LogLevel;

    // backtest params
    char            StartDate[16];
    char            EndDate[16];
    ZSDataFrequency DataFrequency;
    ZSProductClass  ProductType;
    double          CapitalBase;
    int             FillPolicy;             // current bar close, next bar open

    ztl_array_t     BrokerConf;             // zs_broker_conf_t array
    ztl_array_t     AccountConf;            // zs_account_conf_t array
    ztl_array_t     StrategyConf;           // zs_strategy_conf_t array
    ztl_array_t     TradingConf;            // zs_conf_trading_t array
};


//////////////////////////////////////////////////////////////////////////
/* ��������
 */
int zs_configs_load(zs_algo_param_t* algo_param, ztl_pool_t* pool);


/* ��������
 */
zs_conf_broker_t* zs_configs_find_broker(zs_algo_param_t* algo_param, const char* brokerid);

zs_conf_account_t* zs_configs_find_account(zs_algo_param_t* algo_param, const char* accountid);

zs_conf_strategy_t* zs_configs_find_strategy(zs_algo_param_t* algo_param, const char* strategy_name);



/* TODO: ����ʹ��һ���������ýӿڣ�ʹ���߿��Դ������ļ���ʼ����Ҳ����set_xxx/get_xxx
 * ƽ̨ʹ�øýӿ���Ϊģ�������ò�������
 */

#ifdef __cplusplus
}
#endif

#endif//_ZS_CONFIGS_H_INCLUDED_

#pragma once

#include <ThostFtdcMdApi.h>

#include <ZStrategyAPI/zs_broker_api.h>


class ZSCtpMdSpi : public CThostFtdcMdSpi
{
public:
    ZSCtpMdSpi(CThostFtdcMdApi* apMdApi);
    virtual ~ZSCtpMdSpi();

public:
    virtual void OnFrontConnected();
    virtual void OnFrontDisconnected(int nReason);

    ///��¼������Ӧ
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�ǳ�������Ӧ
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///����Ӧ��
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///��������Ӧ��
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///ȡ����������Ӧ��
    virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�������֪ͨ
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

public:
    CThostFtdcMdApi*        m_pMdApi;
    zs_md_api_handlers_t*   m_Handlers;
    void*                   m_zsMdCtx;
    int m_RequestID;
};


//////////////////////////////////////////////////////////////////////////
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
ZS_CTP_API int md_api_entry(zs_md_api_t* mdapi);


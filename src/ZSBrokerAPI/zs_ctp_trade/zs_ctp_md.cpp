#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ThostFtdcMdApi.h>

#include <ZStrategyAPI/zs_broker_api.h>

#include "zs_ctp_common.h"



class ZSCtpMdSpi : public CThostFtdcMdSpi
{
public:
    ZSCtpMdSpi(CThostFtdcMdApi* apMdApi);
    virtual ~ZSCtpMdSpi();

public:
    virtual void OnFrontConnected();
    virtual void OnFrontDisconnected(int nReason);

    ///登录请求响应
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///登出请求响应
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///错误应答
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///订阅行情应答
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///取消订阅行情应答
    virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///深度行情通知
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

public:
    CThostFtdcMdApi*        m_pMdApi;
    zs_md_api_handlers_t*   m_Handlers;
    void*                   m_zsMdCtx;
    int m_RequestID;
};


void* md_create(const char* str, int reserve)
{
    CThostFtdcMdApi* mdapi;
    ZSCtpMdSpi* mdspi;

    mdapi = CThostFtdcMdApi::CreateFtdcMdApi(str);
    mdspi = new ZSCtpMdSpi(mdapi);

    printf("ctp md api %s\n", mdspi->m_pMdApi->GetApiVersion());
    mdspi->m_pMdApi->RegisterSpi(mdspi);
    return mdspi;
}

void md_release(void* apiInstance)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)apiInstance;

    if (mdspi->m_pMdApi)
    {
        mdspi->m_pMdApi->RegisterSpi(NULL);
        mdspi->m_pMdApi->Release();
        mdspi->m_pMdApi = NULL;
    }

    delete mdspi;
}

void md_regist(void* apiInstance, zs_md_api_handlers_t* handlers,
    void* mdCtx, const zs_broker_conf_t* apiConf)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)apiInstance;

    mdspi->m_Handlers = handlers;
    mdspi->m_zsMdCtx = mdCtx;
}

void md_connect(void* apiInstance, void* addr)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)apiInstance;

    mdspi->m_pMdApi->RegisterFront(0);

    mdspi->m_pMdApi->Init();
}

int md_login(void* apiInstance)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)apiInstance;

    CThostFtdcReqUserLoginField lLogin = { 0 };
    return mdspi->m_pMdApi->ReqUserLogin(&lLogin, ++mdspi->m_RequestID);
}

int md_subscribe(void* apiInstance, char* ppInstruments[], int count)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)apiInstance;

    return mdspi->m_pMdApi->SubscribeMarketData(ppInstruments, count);
}

int md_unsubscribe(void* apiInstance, char* ppInstruments[], int count)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)apiInstance;

    return mdspi->m_pMdApi->UnSubscribeMarketData(ppInstruments, count);
}


//////////////////////////////////////////////////////////////////////////
ZSCtpMdSpi::ZSCtpMdSpi(CThostFtdcMdApi* apMdApi)
    : m_pMdApi(apMdApi)
    , m_Handlers()
    , m_zsMdCtx()
    , m_RequestID()
{}

ZSCtpMdSpi::~ZSCtpMdSpi()
{}

void ZSCtpMdSpi::OnFrontConnected()
{
    // todo: request login
    if (m_Handlers->on_connect)
        m_Handlers->on_connect(m_zsMdCtx, 0);
}

void ZSCtpMdSpi::OnFrontDisconnected(int nReason)
{
    if (m_Handlers->on_connect)
        m_Handlers->on_connect(m_zsMdCtx, nReason);
}

void ZSCtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    // auto re-subscribe?
    zs_login_t loginRsp = { 0 };
    strcpy(loginRsp.AccountID, pRspUserLogin->UserID);

    if (m_Handlers->on_login)
        m_Handlers->on_login(m_zsMdCtx, &loginRsp);
}

void ZSCtpMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pMD)
{
    zs_tick_t zTick = { 0 };
    strcpy(zTick.Symbol, pMD->InstrumentID);

    if (m_Handlers->on_marketdata)
        m_Handlers->on_marketdata(m_zsMdCtx, &zTick);
}

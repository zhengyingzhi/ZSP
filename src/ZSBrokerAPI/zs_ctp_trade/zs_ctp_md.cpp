#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ThostFtdcMdApi.h>

#include <ZStrategyAPI/zs_broker_api.h>

#include "zs_ctp_common.h"


#ifdef ZS_HAVE_SE
#pragma comment(lib, "thostmduserapi_se.lib")
#else
#pragma comment(lib, "thostmduserapi.lib")
#endif//ZS_HAVE_SE


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

void md_release(void* instance)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    if (mdspi->m_pMdApi)
    {
        mdspi->m_pMdApi->RegisterSpi(NULL);
        mdspi->m_pMdApi->Release();
        mdspi->m_pMdApi = NULL;
    }

    delete mdspi;
}

void md_regist(void* instance, zs_md_api_handlers_t* handlers,
    void* mdctx, const zs_conf_broker_t* conf)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    mdspi->m_Handlers = handlers;
    mdspi->m_zsMdCtx = mdctx;
}

void md_connect(void* instance, void* addr)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    mdspi->m_pMdApi->RegisterFront(0);

    mdspi->m_pMdApi->Init();
}

int md_login(void* instance)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    CThostFtdcReqUserLoginField lLogin = { 0 };
    return mdspi->m_pMdApi->ReqUserLogin(&lLogin, ++mdspi->m_RequestID);
}

int md_subscribe(void* instance, char* ppInstruments[], int count)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    return mdspi->m_pMdApi->SubscribeMarketData(ppInstruments, count);
}

int md_unsubscribe(void* instance, char* ppInstruments[], int count)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

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
        m_Handlers->on_connect(m_zsMdCtx);
}

void ZSCtpMdSpi::OnFrontDisconnected(int nReason)
{
    if (m_Handlers->on_disconnect)
        m_Handlers->on_disconnect(m_zsMdCtx, nReason);
}

void ZSCtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    // auto re-subscribe?
    zs_login_t loginRsp = { 0 };
    strcpy(loginRsp.AccountID, pRspUserLogin->UserID);

    if (m_Handlers->on_login)
        m_Handlers->on_login(m_zsMdCtx, &loginRsp, NULL);
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

    if (m_Handlers->on_rtn_mktdata)
        m_Handlers->on_rtn_mktdata(m_zsMdCtx, &zTick);
}

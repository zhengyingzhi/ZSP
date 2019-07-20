#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <ThostFtdcMdApi.h>

#include <ZStrategyAPI/zs_broker_api.h>

#include "zs_ctp_common.h"

#include "zs_ctp_md.h"


#ifdef ZS_HAVE_SE
#pragma comment(lib, "thostmduserapi_se.lib")
#else
#pragma comment(lib, "thostmduserapi.lib")
#endif//ZS_HAVE_SE


extern void conv_rsp_info(zs_error_data_t* error, CThostFtdcRspInfoField *pRspInfo);
extern int32_t conv_ctp_time(const char* stime);


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
    if (pRspUserLogin) {
        strcpy(loginRsp.BrokerID, pRspUserLogin->BrokerID);
        strcpy(loginRsp.AccountID, pRspUserLogin->UserID);
        loginRsp.TradingDay = atoi(pRspUserLogin->TradingDay);
    }

    if (m_Handlers->on_login)
        m_Handlers->on_login(m_zsMdCtx, &loginRsp, NULL);
}

void ZSCtpMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    zs_error_data_t error;
    conv_rsp_info(&error, pRspInfo);
    if (m_Handlers->on_rsp_error)
        m_Handlers->on_rsp_error(m_zsMdCtx, &error);
}

void ZSCtpMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pMD)
{
    zs_tick_t ztick = { 0 };
    ztick.TradingDay = atoi(pMD->TradingDay);
    ztick.ActionDay = atoi(pMD->ActionDay);

    // TODO: process error action day
    // TODO: filter invalid md

    // ztick.ExchangeID = zs_convert_exchange_name(pMD->ExchangeID);
    strcpy(ztick.Symbol, pMD->InstrumentID);

    ztick.LastPrice = pMD->LastPrice;
    ztick.OpenPrice = pMD->OpenPrice;
    ztick.HighPrice = pMD->HighestPrice;
    ztick.LowPrice = pMD->LowestPrice;
    ztick.Volume = pMD->Volume;
    ztick.Turnover = pMD->Turnover;
    ztick.OpenInterest = pMD->OpenInterest;

    ztick.SettlementPrice = pMD->SettlementPrice;
    ztick.UpperLimit = pMD->UpperLimitPrice;
    ztick.LowerLimit = pMD->LowerLimitPrice;

    ztick.PreClosePrice = pMD->PreClosePrice;
    ztick.PreOpenInterest = pMD->PreOpenInterest;
    ztick.PreSettlementPrice = pMD->PreSettlementPrice;
    ztick.PreDelta = pMD->PreDelta;
    ztick.CurrDelta = pMD->CurrDelta;

    ztick.UpdateTime = conv_ctp_time(pMD->UpdateTime) * 1000 + pMD->UpdateMillisec;
    ztick.BidPrice[0] = pMD->BidPrice1;
    ztick.BidVolume[0] = pMD->BidVolume1;
    ztick.AskPrice[0] = pMD->AskPrice1;
    ztick.AskVolume[0] = pMD->AskVolume1;

    if (m_Handlers->on_rtn_mktdata)
        m_Handlers->on_rtn_mktdata(m_zsMdCtx, &ztick);
}

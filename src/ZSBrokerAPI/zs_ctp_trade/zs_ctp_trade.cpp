#include <string.h>
#include <stdlib.h>

#include <ThostFtdcTraderApi.h>

#include <ZStrategyAPI/zs_broker_api.h>

#include "zs_ctp_common.h"


class ZSCtpTradeSpi : public CThostFtdcTraderSpi
{
public:
    ZSCtpTradeSpi(CThostFtdcTraderApi* apTradeApi);
    virtual ~ZSCtpTradeSpi();

    int NextReqID() {
        return m_RequestID++;
    }

public:
    virtual void OnFrontConnected();

    virtual void OnFrontDisconnected(int nReason);

    ///�ͻ�����֤��Ӧ
    virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

    ///��¼������Ӧ
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�ǳ�������Ӧ
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///����¼��������Ӧ
    virtual void OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///��������������Ӧ
    virtual void OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///��ѯ��󱨵�������Ӧ
    virtual void OnRspQueryMaxOrderVolume(CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

    ///Ͷ���߽�����ȷ����Ӧ
    virtual void OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ������Ӧ
    virtual void OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ�ɽ���Ӧ
    virtual void OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯͶ���ֲ߳���Ӧ
    virtual void OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ�ʽ��˻���Ӧ
    virtual void OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯͶ������Ӧ
    virtual void OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ���ױ�����Ӧ
    virtual void OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ��Լ��֤������Ӧ
    virtual void OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ��Լ����������Ӧ
    virtual void OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ��������Ӧ
    virtual void OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ��Ʒ��Ӧ
    virtual void OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ��Լ��Ӧ
    virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�����ѯ������Ӧ
    virtual void OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///����Ӧ��
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///����֪ͨ
    virtual void OnRtnOrder(CThostFtdcOrderField *pOrder);

    ///�ɽ�֪ͨ
    virtual void OnRtnTrade(CThostFtdcTradeField *pTrade);

    ///����¼�����ر�
    virtual void OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo);

    ///������������ر�
    virtual void OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo);

    ///��Լ����״̬֪ͨ
    virtual void OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus);

public:
    CThostFtdcTraderApi*        m_pTradeApi;
    zs_trade_api_handlers_t*    m_Handlers;
    void*   m_zsTdCtx;
    int     m_RequestID;
};


void* trade_create(const char* str, int reserve)
{
    ZSCtpTradeSpi*          tdspi;
    CThostFtdcTraderApi*    tdapi;

    tdapi = CThostFtdcTraderApi::CreateFtdcTraderApi(str);
    tdspi = new ZSCtpTradeSpi(tdapi);

    printf("ctp trade api %s\n", tdspi->m_pTradeApi->GetApiVersion());
    tdspi->m_pTradeApi->RegisterSpi(tdspi);
    return NULL;
}

void trade_release(void* apiInstance)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)apiInstance;

    if (tdspi->m_pTradeApi)
    {
        tdspi->m_pTradeApi->RegisterSpi(NULL);
        tdspi->m_pTradeApi->Release();
        tdspi->m_pTradeApi = NULL;
    }

    delete tdspi;
}

void trade_regist(void* apiInstance, zs_trade_api_handlers_t* handlers,
    void* tdCtx, const zs_broker_conf_t* apiConf)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)apiInstance;

    tdspi->m_Handlers = handlers;
    tdspi->m_zsTdCtx = tdCtx;
}

void trade_connect(void* apiInstance, void* addr)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)apiInstance;

    tdspi->m_pTradeApi->RegisterFront(0);

    tdspi->m_pTradeApi->Init();
}

int trade_order(void* apiInstance, const zs_order_t* orderReq)
{
    CThostFtdcInputOrderField lOrder = { 0 };
    strcpy(lOrder.InstrumentID, orderReq->Symbol);

    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)apiInstance;

    int rv;
    rv = tdspi->m_pTradeApi->ReqOrderInsert(&lOrder, tdspi->NextReqID());

    return rv;
}

int trade_cancel(void* apiInstance, const zs_cancel_req_t* cancelReq)
{
    CThostFtdcInputOrderActionField lAction= { 0 };
    strcpy(lAction.InstrumentID, cancelReq->Symbol);

    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)apiInstance;

    int rv;
    rv = tdspi->m_pTradeApi->ReqOrderAction(&lAction, tdspi->NextReqID());

    return rv;
}


//////////////////////////////////////////////////////////////////////////

ZSCtpTradeSpi::ZSCtpTradeSpi(CThostFtdcTraderApi* apTradeApi)
    : m_pTradeApi(apTradeApi)
    , m_Handlers()
    , m_zsTdCtx()
    , m_RequestID(1)
{
    //
}

ZSCtpTradeSpi::~ZSCtpTradeSpi()
{}

void ZSCtpTradeSpi::OnFrontConnected()
{
    // auto request login ?
    if (m_Handlers->on_connect)
        m_Handlers->on_connect(m_zsTdCtx, 0);
}

void ZSCtpTradeSpi::OnFrontDisconnected(int nReason)
{
    if (m_Handlers->on_connect)
        m_Handlers->on_connect(m_zsTdCtx, nReason);
}

void ZSCtpTradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    // login rsp
    zs_login_t loginRsp = { 0 };
    strcpy(loginRsp.AccountID, pRspUserLogin->UserID);

    if (m_Handlers->on_login)
        m_Handlers->on_login(m_zsTdCtx, &loginRsp);
}

void ZSCtpTradeSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryInvestor(CThostFtdcInvestorField *pInvestor, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryProduct(CThostFtdcProductField *pProduct, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    zs_order_t zOrder = { 0 };
    strcpy(zOrder.Symbol, pOrder->InstrumentID);
    strcpy(zOrder.AccountID, pOrder->InvestorID);

    zOrder.Price = (float)pOrder->LimitPrice;
    zOrder.Quantity = pOrder->VolumeTotal;

    if (m_Handlers->on_order)
        m_Handlers->on_order(m_zsTdCtx, &zOrder);
}

void ZSCtpTradeSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    zs_trade_t zTrade = { 0 };
    strcpy(zTrade.Symbol, pTrade->InstrumentID);

    if (m_Handlers->on_trade)
        m_Handlers->on_trade(m_zsTdCtx, &zTrade);
}

void ZSCtpTradeSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{}

void ZSCtpTradeSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{}

void ZSCtpTradeSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
{}

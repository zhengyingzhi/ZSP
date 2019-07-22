#include <string.h>
#include <stdlib.h>

#include <ZStrategyAPI/zs_constants_helper.h>

#include "zs_ctp_common.h"

#include "zs_ctp_trade.h"

#ifdef ZS_HAVE_SE
#pragma comment(lib, "thosttraderapi_se.lib")
#else
#pragma comment(lib, "thosttraderapi.lib")
#endif//ZS_HAVE_SE


extern void conv_rsp_info(zs_error_data_t* error, CThostFtdcRspInfoField *pRspInfo);
extern int32_t conv_ctp_time(const char* stime);


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

void trade_release(void* instance)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    if (tdspi->m_pTradeApi)
    {
        tdspi->m_pTradeApi->RegisterSpi(NULL);
        tdspi->m_pTradeApi->Release();
        tdspi->m_pTradeApi = NULL;
    }

    delete tdspi;
}

void trade_regist(void* instance, zs_trade_api_handlers_t* handlers,
    void* tdctx, const zs_conf_broker_t* conf)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    tdspi->m_Handlers = handlers;
    tdspi->m_zsTdCtx = tdctx;
}

void trade_connect(void* instance, void* addr)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    tdspi->m_pTradeApi->RegisterFront(0);

    tdspi->m_pTradeApi->Init();
}

int trade_order(void* instance, const zs_order_t* order_req)
{
    CThostFtdcInputOrderField input_order = { 0 };
    strcpy(input_order.InstrumentID, order_req->Symbol);

    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    int rv;
    rv = tdspi->m_pTradeApi->ReqOrderInsert(&input_order, tdspi->NextReqID());

    return rv;
}

int trade_cancel(void* instance, const zs_cancel_req_t* cancelReq)
{
    CThostFtdcInputOrderActionField lAction= { 0 };
    strcpy(lAction.InstrumentID, cancelReq->Symbol);

    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    int rv;
    rv = tdspi->m_pTradeApi->ReqOrderAction(&lAction, tdspi->NextReqID());

    return rv;
}


int trade_api_entry(zs_trade_api_t* tdapi)
{
    tdapi->create = trade_create;
    tdapi->release = trade_release;
    // tdapi->regist = trade_regist;
    // tdapi->connect = trade_connect;
    return 0;
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
        m_Handlers->on_connect(m_zsTdCtx);
}

void ZSCtpTradeSpi::OnFrontDisconnected(int nReason)
{
    if (m_Handlers->on_disconnect)
        m_Handlers->on_disconnect(m_zsTdCtx, nReason);
}

void ZSCtpTradeSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    zs_error_data_t error;
    zs_authenticate_t auth = { 0 };

    if (pRspAuthenticateField)
    {
        strcpy(auth.AccountID, pRspAuthenticateField->UserID);
        strcpy(auth.BrokerID, pRspAuthenticateField->BrokerID);
        strcpy(auth.UserProductInfo, pRspAuthenticateField->UserProductInfo);
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_authenticate)
        m_Handlers->on_authenticate(m_zsTdCtx, &auth, &error);
}

void ZSCtpTradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    // login rsp
    zs_error_data_t error;
    zs_login_t zlogin = { 0 };

    if (pRspUserLogin)
    {
        strcpy(zlogin.BrokerID, pRspUserLogin->BrokerID);
        strcpy(zlogin.AccountID, pRspUserLogin->UserID);

        zlogin.TradingDay = atoi(pRspUserLogin->TradingDay);
        zlogin.LoginTime = conv_ctp_time(pRspUserLogin->LoginTime);
        zlogin.SHFETime = conv_ctp_time(pRspUserLogin->SHFETime);
        zlogin.DCETime = conv_ctp_time(pRspUserLogin->DCETime);
        zlogin.CZCETime = conv_ctp_time(pRspUserLogin->CZCETime);
        zlogin.CFFEXTime = conv_ctp_time(pRspUserLogin->FFEXTime);
        zlogin.INETime = conv_ctp_time(pRspUserLogin->INETime);

        zlogin.MaxOrderRef = atoi(pRspUserLogin->MaxOrderRef);

        zlogin.FrontID = pRspUserLogin->FrontID;
        zlogin.SessionID = pRspUserLogin->SessionID;
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_login)
        m_Handlers->on_login(m_zsTdCtx, &zlogin, &error);
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
{
    zs_error_data_t error;
    zs_contract_t contract = { 0 };
    if (pInstrument)
    {
        // contract.ExchangeID = zs_convert_exchange_name(pInstrument->ExchangeID);
        strcpy(contract.Symbol, pInstrument->InstrumentID);
        contract.PriceTick = pInstrument->PriceTick;
        contract.Multiplier = pInstrument->VolumeMultiple;
        contract.LongMarginRateByMoney = pInstrument->LongMarginRatio;
        contract.ShortMarginRateByMoney = pInstrument->ShortMarginRatio;
        contract.ProductClass = ZS_PC_Future;
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_rsp_data)
        m_Handlers->on_rsp_data(m_zsTdCtx, ZS_DT_QryContract, &contract, sizeof(contract), &error, bIsLast);
}

void ZSCtpTradeSpi::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    zs_error_data_t error;
    conv_rsp_info(&error, pRspInfo);
    if (m_Handlers->on_rsp_error)
        m_Handlers->on_rsp_error(m_zsTdCtx, &error);
}

void ZSCtpTradeSpi::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    zs_order_t zOrder = { 0 };
    strcpy(zOrder.BrokerID, pOrder->BrokerID);
    strcpy(zOrder.AccountID, pOrder->InvestorID);
    strcpy(zOrder.UserID, pOrder->UserID);
    strcpy(zOrder.Symbol, pOrder->InstrumentID);
    strcpy(zOrder.OrderID, pOrder->OrderRef);
    strcpy(zOrder.OrderSysID, pOrder->OrderSysID);
    strcpy(zOrder.BranchID, pOrder->BranchID);

    // zOrder.ExchangeID = zs_convert_exchange_name(pOrder->ExchangeID);
    zOrder.OrderPrice = pOrder->LimitPrice;
    zOrder.OrderQty = pOrder->VolumeTotal;
    zOrder.FilledQty = pOrder->VolumeTraded;
    zOrder.FrontID = pOrder->FrontID;
    zOrder.SessionID = pOrder->SessionID;

    // TODO: convert
    // zOrder.Direction = pOrder->Direction;
    // zOrder.OffsetFlag = pOrder->CombOffsetFlag[0];

    if (m_Handlers->on_rtn_order)
        m_Handlers->on_rtn_order(m_zsTdCtx, &zOrder);
}

void ZSCtpTradeSpi::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    zs_trade_t zTrade = { 0 };
    strcpy(zTrade.BrokerID, pTrade->BrokerID);
    strcpy(zTrade.AccountID, pTrade->InvestorID);
    strcpy(zTrade.UserID, pTrade->UserID);
    strcpy(zTrade.Symbol, pTrade->InstrumentID);
    strcpy(zTrade.OrderID, pTrade->OrderRef);
    strcpy(zTrade.OrderSysID, pTrade->OrderSysID);
    strcpy(zTrade.TradeID, pTrade->TradeID);

    zTrade.Volume = pTrade->Volume;
    zTrade.Price = pTrade->Price;

    // TODO: convert
    // zTrade.Direction = pTrade->Direction;
    // zTrade.OffsetFlag = pTrade->CombOffsetFlag[0];

    if (m_Handlers->on_rtn_trade)
        m_Handlers->on_rtn_trade(m_zsTdCtx, &zTrade);
}

void ZSCtpTradeSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{}

void ZSCtpTradeSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{}

void ZSCtpTradeSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
{}

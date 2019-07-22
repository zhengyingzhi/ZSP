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


#define my_max(x, y)  ((x) > (y) ? (x) : (y))

static inline char zs2ctp_direction(ZSDirection zdirection)
{
    switch (zdirection)
    {
    case ZS_D_Long:     return THOST_FTDC_D_Buy;
    case ZS_D_Short:    return THOST_FTDC_D_Sell;
    default:            return ' ';
    }
}

static inline char zs2ctp_offset(ZSOffsetFlag zoffset)
{
    switch (zoffset)
    {
    case ZS_OF_Open:        return THOST_FTDC_OF_Open;
    case ZS_OF_Close:       return THOST_FTDC_OF_Close;
    case ZS_OF_CloseToday:  return THOST_FTDC_OF_CloseToday;
    case ZS_OF_CloseYd:     return THOST_FTDC_OF_CloseYesterday;
    default:                return ' ';
    }
}

static inline char zs2ctp_pricetype(ZSOrderType zot)
{
    switch (zot)
    {
    case ZS_OT_Limit:       return THOST_FTDC_OPT_LimitPrice;
    case ZS_OT_Market:      return THOST_FTDC_OPT_AnyPrice;
    default:                return THOST_FTDC_OPT_LimitPrice;
    }
}

static inline ZSDirection ctp2zs_direction(char cdirection)
{
    switch (cdirection)
    {
    case THOST_FTDC_D_Buy:  return ZS_D_Long;
    case THOST_FTDC_D_Sell: return ZS_D_Short;
    default:                return ZS_D_Unknown;
    }
}

static inline ZSOffsetFlag ctp2zs_offset(char coffset)
{
    switch (coffset)
    {
    case THOST_FTDC_OF_Open:        return ZS_OF_Open;
    case THOST_FTDC_OF_Close:       return ZS_OF_Close;
    case THOST_FTDC_OF_CloseToday:  return ZS_OF_CloseToday;
    case THOST_FTDC_OF_CloseYesterday:     return ZS_OF_CloseYd;
    default:                return ZS_OF_Unkonwn;
    }
}

static inline ZSOrderType ctp2zs_pricetype(char cpt)
{
    switch (cpt)
    {
    case THOST_FTDC_OPT_LimitPrice: return ZS_OT_Limit;
    case THOST_FTDC_OPT_AnyPrice:   return ZS_OT_Market;
    default:                        return ZS_OT_Limit;
    }
}

static inline ZSOrderStatus ctp2zs_order_status(char order_status)
{
    switch (order_status)
    {
    case THOST_FTDC_OST_AllTraded: return ZS_OS_Filled;
    case THOST_FTDC_OST_PartTradedQueueing:   return ZS_OS_PartFilled;
    case THOST_FTDC_OST_PartTradedNotQueueing: return ZS_OS_PartCancled;
    case THOST_FTDC_OST_NoTradeQueueing:    return ZS_OS_Accepted;
    case THOST_FTDC_OST_Canceled: return ZS_OS_Canceld;
    case THOST_FTDC_OST_Unknown: return ZS_OS_NotSubmit;
    default:                        return ZS_OS_Rejected;
    }
}

//////////////////////////////////////////////////////////////////////////

extern void conv_rsp_info(zs_error_data_t* error, CThostFtdcRspInfoField *pRspInfo);
extern int32_t conv_ctp_time(const char* stime);


const char* trade_version(void* instance, int* pver)
{
    (void)pver;
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    return tdspi->m_pTradeApi->GetApiVersion();
}

void* trade_create(const char* str, int reserve)
{
    ZSCtpTradeSpi*          tdspi;
    CThostFtdcTraderApi*    tdapi;

    tdapi = CThostFtdcTraderApi::CreateFtdcTraderApi(str);
    tdspi = new ZSCtpTradeSpi(tdapi);

    printf("ctp trade api %s\n", tdspi->m_pTradeApi->GetApiVersion());
    tdspi->m_pTradeApi->RegisterSpi(tdspi);
    return tdspi;
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

int trade_regist(void* instance, zs_trade_api_handlers_t* handlers,
    void* tdctx, const zs_conf_account_t* conf)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    tdspi->m_Handlers = handlers;
    tdspi->m_zsTdCtx = tdctx;
    if (conf) {
        tdspi->m_Conf = *conf;
    }

    return 0;
}

int trade_connect(void* instance, void* addr)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    tdspi->m_pTradeApi->RegisterFront(tdspi->m_Conf.TradeAddr);

    tdspi->m_pTradeApi->Init();

    return 0;
}

int trade_auth(void* instance)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    return tdspi->ReqAuthenticate();
}

int trade_login(void* instance)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    return tdspi->ReqLogin();
}

int trade_logout(void* instance)
{
    (void)instance;
    return -1;
}

int trade_order(void* instance, zs_order_req_t* order_req)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    CThostFtdcInputOrderField input_order = { 0 };
    strcpy(input_order.BrokerID, order_req->BrokerID);
    strcpy(input_order.InvestorID, order_req->AccountID);
    strcpy(input_order.UserID, order_req->UserID);
    strcpy(input_order.InstrumentID, order_req->Symbol);
    // order_ref
    input_order.OrderPriceType = zs2ctp_pricetype(order_req->OrderType);
    input_order.Direction = zs2ctp_direction(order_req->Direction);
    input_order.CombOffsetFlag[0] = zs2ctp_offset(order_req->OffsetFlag);
    input_order.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
    input_order.LimitPrice = order_req->OrderPrice;
    input_order.VolumeTotalOriginal = order_req->OrderQty;
    input_order.TimeCondition = THOST_FTDC_TC_GFD;
    // input_order.GTDDate =  // trading day ? 
    input_order.VolumeCondition = THOST_FTDC_VC_AV;
    input_order.MinVolume = 1;
    input_order.ContingentCondition = THOST_FTDC_CC_Immediately;
    input_order.IsAutoSuspend = 0;
    input_order.RequestID = tdspi->NextReqID();
    input_order.UserForceClose = 0;
    input_order.IsSwapOrder = 0;
    strcpy(input_order.ExchangeID, get_exchange_name(order_req->ExchangeID));

    int rv;
    rv = tdspi->ReqOrderInsert(&input_order);
    if (rv == 0)
    {
        strcpy(order_req->OrderID, input_order.OrderRef);
        order_req->FrontID = tdspi->m_FrontID;
        order_req->SessionID = tdspi->m_SessionID;
    }

    return rv;
}

int trade_cancel(void* instance, zs_cancel_req_t* cancel_req)
{
    ZSCtpTradeSpi* tdspi;
    tdspi = (ZSCtpTradeSpi*)instance;

    CThostFtdcInputOrderActionField lAction= { 0 };
    strcpy(lAction.InstrumentID, cancel_req->Symbol);
    strcpy(lAction.BrokerID, cancel_req->BrokerID);
    strcpy(lAction.InvestorID, cancel_req->AccountID);

    strcpy(lAction.ExchangeID, get_exchange_name(cancel_req->ExchangeID));
    strcpy(lAction.OrderSysID, cancel_req->OrderSysID);

    strcpy(lAction.OrderRef, cancel_req->OrderID);
    lAction.FrontID = cancel_req->FrontID;
    lAction.SessionID = cancel_req->SessionID;

    int rv;
    rv = tdspi->m_pTradeApi->ReqOrderAction(&lAction, tdspi->NextReqID());

    return rv;
}

int trade_query(void* instance, ZSApiQueryCategory category, void* data, int size)
{
    return -1;
}

// the api entry func
int trade_api_entry(zs_trade_api_t* tdapi)
{
    tdapi->ApiName = "CTP";
    tdapi->HLib = NULL;
    tdapi->UserData = NULL;
    tdapi->ApiInstance = NULL;
    tdapi->ApiFlag = 0;

    tdapi->api_version  = trade_version;
    tdapi->create       = trade_create;
    tdapi->release      = trade_release;
    tdapi->regist       = trade_regist;
    tdapi->connect      = trade_connect;
    tdapi->authenticate = trade_auth;
    tdapi->login        = trade_login;
    tdapi->logout       = trade_logout;
    tdapi->order        = trade_order;
    tdapi->cancel       = trade_cancel;
    tdapi->query        = trade_query;
    return 0;
}

//////////////////////////////////////////////////////////////////////////

ZSCtpTradeSpi::ZSCtpTradeSpi(CThostFtdcTraderApi* apTradeApi)
    : m_pTradeApi(apTradeApi)
    , m_Handlers()
    , m_Conf()
    , m_zsTdCtx()
    , m_OrderRef(1)
    , m_FrontID()
    , m_SessionID()
    , m_TradingDay()
    , m_RequestID(1)
{
    //
}

ZSCtpTradeSpi::~ZSCtpTradeSpi()
{}


int ZSCtpTradeSpi::ReqAuthenticate()
{
    CThostFtdcReqAuthenticateField lAuth = { 0 };
    strcpy(lAuth.BrokerID, m_Conf.BrokerID);
    strcpy(lAuth.UserID, m_Conf.AccountID);
    strcpy(lAuth.AppID, m_Conf.AppID);
    strcpy(lAuth.AuthCode, m_Conf.AuthCode);
    return m_pTradeApi->ReqAuthenticate(&lAuth, NextReqID());
}

int ZSCtpTradeSpi::ReqLogin()
{
    CThostFtdcReqUserLoginField lLogin = { 0 };
    strcpy(lLogin.BrokerID, m_Conf.BrokerID);
    strcpy(lLogin.UserID, m_Conf.AccountID);
    strcpy(lLogin.Password, m_Conf.Password);
    return m_pTradeApi->ReqUserLogin(&lLogin, NextReqID());
}

int ZSCtpTradeSpi::ReqOrderInsert(CThostFtdcInputOrderField* input_order)
{
    int rv;

    NextOrderRef(input_order->OrderRef);

    rv = m_pTradeApi->ReqOrderInsert(input_order, input_order->RequestID);
    return rv;
}


//////////////////////////////////////////////////////////////////////////
void ZSCtpTradeSpi::OnFrontConnected()
{
    // auto request login ?
    fprintf(stderr, "td connected\n");
    if (m_Handlers->on_connect)
        m_Handlers->on_connect(m_zsTdCtx);

    if (m_Conf.AuthCode[0]) {
        ReqAuthenticate();
    }
    else {
        ReqLogin();
    }
}

void ZSCtpTradeSpi::OnFrontDisconnected(int nReason)
{
    fprintf(stderr, "td disconnected:%d\n", nReason);
    if (m_Handlers->on_disconnect)
        m_Handlers->on_disconnect(m_zsTdCtx, nReason);
}

void ZSCtpTradeSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    fprintf(stderr, "onrsp auth\n");
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

    if (pRspAuthenticateField) {
        ReqLogin();
    }
}

void ZSCtpTradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    fprintf(stderr, "onrsp login\n");

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

        zlogin.MaxOrderRef = my_max(m_OrderRef, atoi(pRspUserLogin->MaxOrderRef));

        zlogin.FrontID = pRspUserLogin->FrontID;
        zlogin.SessionID = pRspUserLogin->SessionID;

        m_FrontID = pRspUserLogin->FrontID;
        m_SessionID = pRspUserLogin->SessionID;
        m_TradingDay = zlogin.TradingDay;
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_login)
        m_Handlers->on_login(m_zsTdCtx, &zlogin, &error);

    // TODO: auto request settlement confirm
}

void ZSCtpTradeSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    // 报单错误通知
}

void ZSCtpTradeSpi::OnRspOrderAction(CThostFtdcInputOrderActionField *pInputOrderAction, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    // 撤单错误通知
}

void ZSCtpTradeSpi::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryOrder(CThostFtdcOrderField *pOrder, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryTrade(CThostFtdcTradeField *pTrade, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField *pInvestorPosition, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{}

void ZSCtpTradeSpi::OnRspQryTradingAccount(CThostFtdcTradingAccountField *pTradingAccount, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    // 资金账户查询应答
}

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

    zOrder.ExchangeID = get_exchange_id(pOrder->ExchangeID);
    zOrder.OrderPrice = pOrder->LimitPrice;
    zOrder.OrderQty = pOrder->VolumeTotal;
    zOrder.FilledQty = pOrder->VolumeTraded;
    zOrder.FrontID = pOrder->FrontID;
    zOrder.SessionID = pOrder->SessionID;

    zOrder.TradingDay = atoi(pOrder->TradingDay);
    zOrder.OrderDate = atoi(pOrder->InsertDate);
    zOrder.OrderTime = conv_ctp_time(pOrder->InsertTime);
    zOrder.CancelTime = conv_ctp_time(pOrder->CancelTime);

    // convert
    zOrder.Direction = ctp2zs_direction(pOrder->Direction);
    zOrder.OffsetFlag = ctp2zs_offset(pOrder->CombOffsetFlag[0]);
    zOrder.OrderStatus = ctp2zs_order_status(pOrder->OrderStatus);

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

    zTrade.ExchangeID = get_exchange_id(pTrade->ExchangeID);
    zTrade.Volume = pTrade->Volume;
    zTrade.Price = pTrade->Price;

    zTrade.TradingDay = atoi(pTrade->TradingDay);
    zTrade.TradeDate = atoi(pTrade->TradeDate);
    zTrade.TradeTime = conv_ctp_time(pTrade->TradeTime);

    // convert
    zTrade.Direction = ctp2zs_direction(pTrade->Direction);
    zTrade.OffsetFlag = ctp2zs_offset(pTrade->OffsetFlag);

    // ctp without below fields
    zTrade.FrontID = 0;
    zTrade.SessionID = 0;

    if (m_Handlers->on_rtn_trade)
        m_Handlers->on_rtn_trade(m_zsTdCtx, &zTrade);
}

void ZSCtpTradeSpi::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder, CThostFtdcRspInfoField *pRspInfo)
{}

void ZSCtpTradeSpi::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction, CThostFtdcRspInfoField *pRspInfo)
{}

void ZSCtpTradeSpi::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField *pInstrumentStatus)
{}

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <ctype.h>
#include <time.h>

#include <ZStrategyAPI/zs_broker_api.h>
#include <ZStrategyAPI/zs_constants_helper.h>

#include "zs_ctp_common.h"

#include "zs_ctp_md.h"

#ifdef __linux__
#define LOCALTIME_S(x,y) localtime_r(y,x)
#else
#define LOCALTIME_S(x,y) localtime_s(x,y)
#endif

#ifdef ZS_HAVE_SE
#pragma comment(lib, "thostmduserapi_se.lib")
#else
#pragma comment(lib, "thostmduserapi.lib")
#endif//ZS_HAVE_SE


static const char* _CFFEXProducts[] = { "IC", "IF", "IH", "IO", "T", "TF", "TS", "TT", nullptr };
static const char* _SHFEProducts[]  = { "ag", "al", "au", "cu", "bu", "fu", "hc", "ni", "pb", "rb", "ru", "sn", "sp", "wr", "zn", nullptr };
static const char* _DCEProducts[]   = { "a",  "b",  "bb", "c",  "cs", "eg", "fb", "i",  "j",  "jd", "jm", "l",  "m",  "p",  "pp", "v", "y", "msr", nullptr };
static const char* _CZCEProducts[]  = { "AP", "CF", "CJ", "CY", "FG", "JR", "LR", "MA", "ME", "OI", "PM", "RI", "RM", "RS", "SF", "SM", "SR", "TA", "TC", "WH", "UR", "ZC", nullptr };
static const char* _INEProducts[]   = { "sc", "nr", nullptr };


extern const char* get_exchange_name(ZSExchangeID exchangeid);
extern void conv_rsp_info(zs_error_data_t* error, CThostFtdcRspInfoField *pRspInfo);
extern int32_t conv_ctp_time(const char* stime);

static void conv_zs_dt(zs_dt_t* zsdt, int date, int update_time, int millisec)
{
    zsdt->dt.year   = date / 10000;
    zsdt->dt.month  = (date / 10000) % 100;
    zsdt->dt.day    = date % 1000000;
    zsdt->dt.hour   = update_time / 10000;
    zsdt->dt.minute = (update_time / 100) % 100;
    zsdt->dt.second = update_time % 10000;
    zsdt->dt.millisec = millisec;
}

static inline double check_double(double d) {
    return (d == DBL_MAX) ? 0 : d;
}

static inline uint32_t check_int(uint32_t ln) {
    return (ln == INT_MAX || ln == UINT_MAX) ? 0 : ln;
}

static inline int64_t check_int64(int64_t ln) {
    return (ln == INT_MAX || ln == UINT_MAX || ln > 9999999999999999 || ln < -9999999999999999) ? 0 : ln;
}

static inline uint32_t get_variety_int(const char* variety)
{
    uint32_t var_int = 0;
    var_int = uint32_t(variety[0] - '0');

    while (*variety && isalpha(*variety)) {
        var_int = var_int * 10 + uint32_t(*variety - '0');
        variety++;
    }

    return var_int;
}

//////////////////////////////////////////////////////////////////////////

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

int md_regist(void* instance, zs_md_api_handlers_t* handlers,
    void* mdctx, const zs_conf_account_t* conf)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    mdspi->m_Handlers = handlers;
    mdspi->m_MdCtx = mdctx;

    if (conf) {
        mdspi->m_Conf = *conf;
    }

    return 0;
}

int md_connect(void* instance, void* addr)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    mdspi->m_pMdApi->RegisterFront(mdspi->m_Conf.MDAddr);

    mdspi->m_pMdApi->Init();

    return 0;
}

int md_login(void* instance)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    return mdspi->ReqLogin();
}

int md_logout(void* instance)
{
    (void)instance;
    return -1;
}

int md_subscribe(void* instance, zs_subscribe_t* sub_reqs[], int count)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    return mdspi->Subscribe(sub_reqs, count);
}

int md_unsubscribe(void* instance, zs_subscribe_t* unsub_reqs[], int count)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    return mdspi->Unsubscribe(unsub_reqs, count);
}

int md_subscribe_forquote(void* instance, zs_subscribe_t* sub_reqs[], int count)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    return mdspi->SubscribeForQuote(sub_reqs, count);
}

int md_unsubscribe_forquote(void* instance, zs_subscribe_t* unsub_reqs[], int count)
{
    ZSCtpMdSpi* mdspi;
    mdspi = (ZSCtpMdSpi*)instance;

    return mdspi->UnsubscribeForQuote(unsub_reqs, count);
}


int md_api_entry(zs_md_api_t* mdapi)
{
    mdapi->APIName = "CTP";
    mdapi->HLib = NULL;
    mdapi->UserData = NULL;
    mdapi->ApiInstance = NULL;

    mdapi->create   = md_create;
    mdapi->release  = md_release;
    mdapi->regist   = md_regist;
    mdapi->connect  = md_connect;
    mdapi->login    = md_login;
    mdapi->logout   = md_logout;
    mdapi->subscribe= md_subscribe;
    mdapi->unsubscribe = md_unsubscribe;
    return 0;
}

//////////////////////////////////////////////////////////////////////////
ZSCtpMdSpi::ZSCtpMdSpi(CThostFtdcMdApi* apMdApi)
    : m_pMdApi(apMdApi)
    , m_Handlers()
    , m_MdCtx()
    , m_Conf()
    , m_RequestID()
{
    uint32_t var_int;
    uint32_t i;
    for (i = 0; _CFFEXProducts[i]; ++i) {
        var_int = get_variety_int(_CFFEXProducts[i]);
        m_VarietyMap[var_int] = ZS_EI_CFFEX;
    }
    for (i = 0; _SHFEProducts[i]; ++i) {
        var_int = get_variety_int(_SHFEProducts[i]);
        m_VarietyMap[var_int] = ZS_EI_SHFE;
    }
    for (i = 0; _DCEProducts[i]; ++i) {
        var_int = get_variety_int(_DCEProducts[i]);
        m_VarietyMap[var_int] = ZS_EI_DCE;
    }
    for (i = 0; _CZCEProducts[i]; ++i) {
        var_int = get_variety_int(_CZCEProducts[i]);
        m_VarietyMap[var_int] = ZS_EI_CZCE;
    }
    for (i = 0; _INEProducts[i]; ++i) {
        var_int = get_variety_int(_INEProducts[i]);
        m_VarietyMap[var_int] = ZS_EI_INE;
    }
}

ZSCtpMdSpi::~ZSCtpMdSpi()
{}

int ZSCtpMdSpi::ReqLogin()
{
    CThostFtdcReqUserLoginField lLogin = { 0 };
    strcpy(lLogin.BrokerID, m_Conf.BrokerID);
    strcpy(lLogin.UserID, m_Conf.AccountID);
    strcpy(lLogin.Password, m_Conf.Password);
    return m_pMdApi->ReqUserLogin(&lLogin, ++m_RequestID);
}

int ZSCtpMdSpi::Subscribe(zs_subscribe_t* sub_reqs[], int count)
{
    std::unique_lock<std::mutex> lk(m_Mutex);

    int i;
    char* ppinstruments[1024] = { 0 };
    for (i = 0; i < count && i < 1024; ++i)
    {
        ppinstruments[i] = sub_reqs[i]->Symbol;
        if (m_SubedMap.count(ppinstruments[i]))
            continue;
        m_SubedMap[std::string(ppinstruments[i])] = *sub_reqs[i];
    }

    if (i > 0)
        return m_pMdApi->SubscribeMarketData(ppinstruments, count);
    return 0;
}

int ZSCtpMdSpi::Unsubscribe(zs_subscribe_t* unsub_reqs[], int count)
{
    char* ppinstruments[1024] = { 0 };
    for (int i = 0; i < count && i < 1024; ++i)
    {
        ppinstruments[i] = unsub_reqs[i]->Symbol;
    }

    return m_pMdApi->UnSubscribeMarketData(ppinstruments, count);
}

int ZSCtpMdSpi::SubscribeForQuote(zs_subscribe_t* sub_reqs[], int count)
{
    char* ppinstruments[1024] = { 0 };
    for (int i = 0; i < count && i < 1024; ++i)
    {
        ppinstruments[i] = sub_reqs[i]->Symbol;
    }

    return m_pMdApi->SubscribeForQuoteRsp(ppinstruments, count);
}

int ZSCtpMdSpi::UnsubscribeForQuote(zs_subscribe_t* unsub_reqs[], int count)
{
    char* ppinstruments[1024] = { 0 };
    for (int i = 0; i < count && i < 1024; ++i)
    {
        ppinstruments[i] = unsub_reqs[i]->Symbol;
    }

    return m_pMdApi->UnSubscribeForQuoteRsp(ppinstruments, count);
}


void ZSCtpMdSpi::OnFrontConnected()
{
    // todo: request login
    if (m_Handlers->on_connect)
        m_Handlers->on_connect(m_MdCtx);

    // auto relogin
    CThostFtdcReqUserLoginField lLogin = { 0 };
    strcpy(lLogin.BrokerID, m_Conf.BrokerID);
    strcpy(lLogin.UserID, m_Conf.AccountID);
    strcpy(lLogin.Password, m_Conf.Password);
    m_pMdApi->ReqUserLogin(&lLogin, m_RequestID++);
}

void ZSCtpMdSpi::OnFrontDisconnected(int nReason)
{
    if (m_Handlers->on_disconnect)
        m_Handlers->on_disconnect(m_MdCtx, nReason);
}

void ZSCtpMdSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    // auto re-subscribe?
    zs_error_data_t error;
    zs_login_t zlogin = { 0 };

    if (pRspUserLogin)
    {
        fprintf(stderr, "MD OnRspUserLogin accnt:%s,td:%s\n", pRspUserLogin->UserID, pRspUserLogin->TradingDay);
        strcpy(zlogin.BrokerID, pRspUserLogin->BrokerID);
        strcpy(zlogin.AccountID, pRspUserLogin->UserID);
        zlogin.TradingDay = atoi(pRspUserLogin->TradingDay);
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_login)
        m_Handlers->on_login(m_MdCtx, &zlogin, &error);

    // auto do subscribe ...
#if 1
    std::unique_lock<std::mutex> lk(m_Mutex);
    SubscribeMap::iterator iter;
    char* ppinstruments[1024] = { 0 };
    int   count = (int)m_SubedMap.size();
    int   i = 0;
    for (iter = m_SubedMap.begin(); iter != m_SubedMap.end() && i < count; ++iter)
    {
        ++i;
        ppinstruments[i] = iter->second.Symbol;
    }
    m_pMdApi->SubscribeMarketData(ppinstruments, count);
#else
    char* pInstrument[] = { "rb1910" };
    m_pMdApi->SubscribeMarketData(pInstrument, 1);
#endif
}

void ZSCtpMdSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    zs_error_data_t error;
    zs_logout_t zlogout = { 0 };

    if (pUserLogout)
    {
        strcpy(zlogout.BrokerID, pUserLogout->BrokerID);
        strcpy(zlogout.AccountID, pUserLogout->UserID);
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_logout)
        m_Handlers->on_logout(m_MdCtx, &zlogout, &error);
}

void ZSCtpMdSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    zs_error_data_t error;
    conv_rsp_info(&error, pRspInfo);
    if (m_Handlers->on_rsp_error)
        m_Handlers->on_rsp_error(m_MdCtx, &error);
}

void ZSCtpMdSpi::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    zs_error_data_t error;
    zs_subscribe_t sub;

    if (pSpecificInstrument)
    {
        strcpy(sub.Symbol, pSpecificInstrument->InstrumentID);

        ZSExchangeID exchangeid;
        uint32_t var_int = get_variety_int(pSpecificInstrument->InstrumentID);
        if (m_VarietyMap.count(var_int))
        {
            exchangeid = m_VarietyMap[var_int];
            strcpy(sub.Exchange, get_exchange_name(exchangeid));
        }
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_subscribe)
        m_Handlers->on_subscribe(m_MdCtx, &sub, bIsLast);
}

void ZSCtpMdSpi::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, 
    CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    zs_error_data_t error;
    zs_subscribe_t  unsub;

    if (pSpecificInstrument)
    {
        strcpy(unsub.Symbol, pSpecificInstrument->InstrumentID);

        ZSExchangeID exchangeid;
        uint32_t var_int = get_variety_int(pSpecificInstrument->InstrumentID);
        if (m_VarietyMap.count(var_int))
        {
            exchangeid = m_VarietyMap[var_int];
            strcpy(unsub.Exchange, get_exchange_name(exchangeid));
        }
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_unsubscribe)
        m_Handlers->on_unsubscribe(m_MdCtx, &unsub, bIsLast);
}

void ZSCtpMdSpi::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    zs_error_data_t error;
    zs_subscribe_t  sub;

    if (pSpecificInstrument)
    {
        strcpy(sub.Symbol, pSpecificInstrument->InstrumentID);

        ZSExchangeID exchangeid;
        uint32_t var_int = get_variety_int(pSpecificInstrument->InstrumentID);
        if (m_VarietyMap.count(var_int))
        {
            exchangeid = m_VarietyMap[var_int];
            strcpy(sub.Exchange, get_exchange_name(exchangeid));
        }
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_subscribe_forquote)
        m_Handlers->on_subscribe_forquote(m_MdCtx, &sub, bIsLast);
}

void ZSCtpMdSpi::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    zs_error_data_t error;
    zs_subscribe_t  unsub;

    if (pSpecificInstrument)
    {
        strcpy(unsub.Symbol, pSpecificInstrument->InstrumentID);

        ZSExchangeID exchangeid;
        uint32_t var_int = get_variety_int(pSpecificInstrument->InstrumentID);
        if (m_VarietyMap.count(var_int))
        {
            exchangeid = m_VarietyMap[var_int];
            strcpy(unsub.Exchange, get_exchange_name(exchangeid));
        }
    }

    conv_rsp_info(&error, pRspInfo);

    if (m_Handlers->on_unsubscribe_forquote)
        m_Handlers->on_unsubscribe_forquote(m_MdCtx, &unsub, bIsLast);
}


void ZSCtpMdSpi::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pMD)
{
    uint32_t var_int = get_variety_int(pMD->InstrumentID);

    zs_tick_t ztick = { 0 };
    ztick.TradingDay = atoi(pMD->TradingDay);
    ztick.ActionDay = atoi(pMD->ActionDay);

    if (!m_VarietyMap.count(var_int)) {
        return;
    }
    ztick.ExchangeID = m_VarietyMap[var_int];

    if (ztick.ExchangeID == ZS_EI_DCE)
    {
        struct tm ltm;
        time_t now = time(NULL);
        LOCALTIME_S(&ltm, &now);

        ztick.ActionDay = ((ltm.tm_year + 1900) * 10000) + ((ltm.tm_mon + 1) * 100) + ltm.tm_mday;
    }

    // TODO: filter invalid md

    strcpy(ztick.Symbol, pMD->InstrumentID);

    ztick.LastPrice     = check_double(pMD->LastPrice);
    ztick.OpenPrice     = check_double(pMD->OpenPrice);
    ztick.HighPrice     = check_double(pMD->HighestPrice);
    ztick.LowPrice      = check_double(pMD->LowestPrice);
    ztick.Volume        = check_int(pMD->Volume);
    ztick.Turnover      = check_double(pMD->Turnover);
    ztick.OpenInterest  = check_double(pMD->OpenInterest);

    ztick.SettlementPrice = check_double(pMD->SettlementPrice);
    ztick.UpperLimit    = check_double(pMD->UpperLimitPrice);
    ztick.LowerLimit    = check_double(pMD->LowerLimitPrice);

    ztick.PreClosePrice = check_double(pMD->PreClosePrice);
    ztick.PreOpenInterest = check_double(pMD->PreOpenInterest);
    ztick.PreSettlementPrice = check_double(pMD->PreSettlementPrice);
    ztick.PreDelta      = check_double(pMD->PreDelta);
    ztick.CurrDelta     = check_double(pMD->CurrDelta);

    ztick.UpdateTime    = conv_ctp_time(pMD->UpdateTime);
    conv_zs_dt(&ztick.TickDt, ztick.ActionDay, ztick.UpdateTime, pMD->UpdateMillisec);
    ztick.UpdateTime    = ztick.UpdateTime * 1000 + pMD->UpdateMillisec;
    ztick.BidPrice[0]   = check_double(pMD->BidPrice1);
    ztick.BidVolume[0]  = check_int(pMD->BidVolume1);
    ztick.AskPrice[0]   = check_double(pMD->AskPrice1);
    ztick.AskVolume[0]  = check_int(pMD->AskVolume1);

    if (m_Handlers->on_rtn_mktdata)
        m_Handlers->on_rtn_mktdata(m_MdCtx, &ztick);
}

void ZSCtpMdSpi::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
    zs_forquote_rsp_t zs_forquote = { 0 };
    strcpy(zs_forquote.Symbol, pForQuoteRsp->InstrumentID);
    strcpy(zs_forquote.ForQuoteSysID, pForQuoteRsp->ForQuoteSysID);
    zs_forquote.ForQuoteTime = atoi(pForQuoteRsp->ForQuoteTime);
    zs_forquote.TradingDay = atoi(pForQuoteRsp->TradingDay);
    zs_forquote.ActionDay = atoi(pForQuoteRsp->ActionDay);
    zs_forquote.ExchangeID = get_exchange_id(pForQuoteRsp->ExchangeID);

    if (m_Handlers->on_rtn_forquote)
        m_Handlers->on_rtn_forquote(m_MdCtx, &zs_forquote);
}

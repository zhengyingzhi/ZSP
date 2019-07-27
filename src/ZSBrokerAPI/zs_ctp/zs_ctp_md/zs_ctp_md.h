#pragma once

#include <map>
#include <string>
#include <mutex>

#undef ZS_HAVE_SE
#ifdef ZS_HAVE_SE
#include <ctp_se/ThostFtdcMdApi.h>
#else
#include <ctp/ThostFtdcMdApi.h>
#endif//ZS_HAVE_SE

#include <ZStrategyAPI/zs_broker_api.h>

#include "zs_ctp_common.h"


typedef std::map<uint32_t, ZSExchangeID>        VarietyMap;
typedef std::map<std::string, zs_subscribe_t>   SubscribeMap;


class ZSCtpMdSpi : public CThostFtdcMdSpi
{
public:
    ZSCtpMdSpi(CThostFtdcMdApi* apMdApi);
    virtual ~ZSCtpMdSpi();

public:
    int ReqLogin();
    int Subscribe(zs_subscribe_t* sub_reqs[], int count);
    int Unsubscribe(zs_subscribe_t* unsub_reqs[], int count);
    int SubscribeForQuote(zs_subscribe_t* sub_reqs[], int count);
    int UnsubscribeForQuote(zs_subscribe_t* unsub_reqs[], int count);

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

    ///����ѯ��Ӧ��
    virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///ȡ������ѯ��Ӧ��
    virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    ///�������֪ͨ
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

    ///ѯ��֪ͨ
    virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp);

public:
    CThostFtdcMdApi*        m_pMdApi;
    zs_md_api_handlers_t*   m_Handlers;
    void*                   m_MdCtx;
    VarietyMap              m_VarietyMap;
    SubscribeMap            m_SubedMap;
    std::mutex              m_Mutex;
    zs_conf_account_t       m_Conf;
    int                     m_RequestID;
};


//////////////////////////////////////////////////////////////////////////
/* md apis */
void* md_create(const char* str, int reserve);

void md_release(void* instance);

int md_regist(void* instance, zs_md_api_handlers_t* handlers,
    void* mdctx, const zs_conf_account_t* conf);

int md_connect(void* instance, void* addr);

int md_login(void* instance);

int md_subscribe(void* instance, zs_subscribe_t* sub_reqs[], int count);

int md_unsubscribe(void* instance, zs_subscribe_t* sub_reqs[], int count);

int md_subscribe_forquote(void* instance, zs_subscribe_t* sub_reqs[], int count);

int md_unsubscribe_forquote(void* instance, zs_subscribe_t* unsub_reqs[], int count);


/* the exported dso entry
 */
#ifdef __cplusplus
extern "C" {
#endif

ZS_CTP_API int md_api_entry(zs_md_api_t* mdapi);


#ifdef __cplusplus
}
#endif

#pragma once

#include <stdio.h>
#include <string.h>

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

    int NextOrderRef(char order_req[]) {
        return sprintf(order_req, "%d", m_OrderRef++);
    }

public:
    int ReqAuthenticate();
    int ReqLogin();

    int ReqOrderInsert(CThostFtdcInputOrderField* input_order);

public:
    virtual void OnFrontConnected();

    virtual void OnFrontDisconnected(int nReason);

    ///�ͻ�����֤��Ӧ
    virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

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
    zs_conf_account_t           m_Conf;
    void*   m_zsTdCtx;
    int     m_OrderRef;
    int     m_FrontID;
    int     m_SessionID;
    int     m_TradingDay;
    int     m_RequestID;
};


/* trader apis */
void* trade_create(const char* str, int reserve);

void trade_release(void* instance);

int trade_regist(void* instance, zs_trade_api_handlers_t* handlers,
    void* tdctx, const zs_conf_account_t* conf);

int trade_connect(void* instance, void* addr);

int trade_auth(void* instance);

int trade_login(void* instance);

int trade_order(void* instance, zs_order_req_t* order_req);

int trade_cancel(void* instance, zs_cancel_req_t* cancel_req);


/* the exported dso entry
 */
#ifdef __cplusplus
extern "C" {
#endif

ZS_CTP_API int trade_api_entry(zs_trade_api_t* tdapi);


#ifdef __cplusplus
}
#endif


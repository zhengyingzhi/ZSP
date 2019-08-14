/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define the common api struct
 */

#ifndef _ZS_API_STRUCT_H_
#define _ZS_API_STRUCT_H_

#include <stdint.h>

#include "zs_constants.h"


#define ORDER_ID_LEN        20
#define ORDER_SYSID_LEN     24
#define TRADE_ID_LEN        24
#define BROKER_ID_LEN       12
#define ACCOUNT_ID_LEN      16
#define BRANCH_ID_LEN       12

#ifdef __cplusplus
extern "C" {
#endif


#pragma pack(push, 1)

struct zs_data_head_s 
{
    struct zs_data_head_s* prev;
    struct zs_data_head_s* next;
    void      (*Cleanup)(struct zs_data_head_s*);
    void*       CtxData;
    const char* pSymbol;            // point to the symbol id of the data body
    uint16_t    SymbolLength;
    uint16_t    ExchangeID;
    volatile    uint32_t RefCount;
    uint32_t    Flags;
    ZSDataType  DType;              // the data type
    uint32_t    Size;               // the body size
    int64_t     HashID;             // symbol hash id
};
typedef struct zs_data_head_s zs_data_head_t;

#define zd_data_body(zdh)       (char*)((zdh) + 1)
#define zd_data_head_size       sizeof(struct zs_data_head_s)
#define zd_data_body_size(zdh)  (zdh)->Size
#define zd_data_total_size(zdh) (sizeof(struct zs_data_head_s) + (zdh)->Size)

extern uint32_t zs_data_increment(zs_data_head_t* zdh);
extern uint32_t zs_data_decre_release(zs_data_head_t* zdh);



typedef union zs_dt_u
{
    struct dt_s
    {
        uint64_t    millisec : 10;
        uint64_t    year : 14;
        uint64_t    month : 8;
        uint64_t    day : 8;
        uint64_t    hour : 8;
        uint64_t    minute : 8;
        uint64_t    second : 8;
    };
    struct dt_s dt;
    uint64_t dt64;
}zs_dt_t;

/* error data */
struct zs_error_data_s
{
    int             ErrorID;
    char            ErrorMsg[82];
};
typedef struct zs_error_data_s zs_error_data_t;

/* authenticate data */
struct zs_authenticate_s
{
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserProductInfo[16];
    int             Result;
};
typedef struct zs_authenticate_s zs_authenticate_t;

/* login */
struct zs_login_s
{
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    int             Result;

    int32_t         TradingDay;
    int32_t         LoginTime;
    int32_t         FrontID;
    int32_t         SessionID;
    int32_t         MaxOrderRef;
    int32_t     	SHFETime;
    int32_t     	DCETime;
    int32_t     	CZCETime;
    int32_t     	CFFEXTime;
    int32_t     	INETime;
};
typedef struct zs_login_s zs_login_t;

/* logout */
struct zs_logout_s
{
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    int             Result;
};
typedef struct zs_logout_s zs_logout_t;

/* level1 market data */
struct zs_tick_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    zs_dt_t         TickDt;
    uint64_t        Sid;
    int32_t         TradingDay;
    int32_t         ActionDay;
    int32_t         UpdateTime;

    double          OpenPrice;
    double          HighPrice;
    double          LowPrice;
    double          LastPrice;
    double          ClosePrice;

    int64_t         Volume;
    double          Turnover;
    double          OpenInterest;
    double          PreOpenInterest;
    double          PreClosePrice;
    double          PreSettlementPrice;
    double          SettlementPrice;

    double          UpperLimit;
    double          LowerLimit;
    double          PreDelta;
    double          CurrDelta;

    double          UpDown;
    int64_t         LastVolume;

    double          BidPrice[5];
    int32_t         BidVolume[5];

    double          AskPrice[5];
    int32_t         AskVolume[5];
};
typedef struct zs_tick_s zs_tick_t;

/* level2 market data */
struct zs_tickl2_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    zs_dt_t         MdDt;
    uint64_t        Sid;
    int32_t         TradingDay;
    int32_t         ActionDay;
    int32_t         UpdateTime;

    double          OpenPrice;
    double          HighPrice;
    double          LowPrice;
    double          LastPrice;

    int64_t         Volume;
    int64_t         Turnover;
    double          OpenInterest;
    double          PreOpenInterest;
    float           PreClosePrice;

    double          UpperLimit;
    double          LowerLimit;
    double          PreDelta;
    double          CurrDelta;

    double          UpDown;
    int32_t         LastVolume;

    double          BidPrice[10];
    int32_t         BidVolume[10];

    double          AskPrice[10];
    int32_t         AskVolume[10];
};
typedef struct zs_tickl2_s zs_tickl2_t;

/* bar */
struct zs_bar_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    int64_t         BarTime;            // 20181201093500
    zs_dt_t         BarDt;
    uint64_t        Sid;

    double          OpenPrice;
    double          HighPrice;
    double          LowPrice;
    double          ClosePrice;

    int64_t         Volume;
    double          Amount;
    double          OpenInterest;       // for Future
    double          SettlePrice;        // for Future
    double          AdjustFactor;       // for Equity
    char            Period[8];          // bar period
};
typedef struct zs_bar_s zs_bar_t;


/* trade data */
struct zs_trade_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];
    
    char            OrderID[ORDER_ID_LEN];
    char            OrderSysID[ORDER_SYSID_LEN];
    char            TradeID[TRADE_ID_LEN];

    uint64_t        Sid;
    double          Price;
    int32_t         Volume;

    ZSDirection     Direction;
    ZSOffsetFlag    OffsetFlag;
    int32_t         TradingDay;
    int32_t         TradeDate;
    int32_t         TradeTime;
    int32_t         IsLast;

    double          RealizedPnl;
    double          CurrMargin;
    double          Commission;

    // ctp 
    int32_t         FrontID;
    int32_t         SessionID;
    char            Padding[12];
};
typedef struct zs_trade_s zs_trade_t;


/* order data */
struct zs_order_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];
    char            BranchID[BRANCH_ID_LEN];

    char            OrderID[ORDER_ID_LEN];
    char            OrderSysID[ORDER_SYSID_LEN];

    uint64_t        Sid;
    double          OrderPrice;
    double          AvgPrice;
    int32_t         OrderQty;
    int32_t         FilledQty;
    ZSDirection     Direction;
    ZSOffsetFlag    OffsetFlag;
    ZSOrderType     OrderType;
    ZSOrderStatus   OrderStatus;
    int32_t         TradingDay;
    int32_t         OrderDate;
    int32_t         OrderTime;
    int32_t         CancelTime;

    // internal fields
    int32_t         Multiplier;
    int32_t         IsLast;

    // ctp 
    int32_t         FrontID;
    int32_t         SessionID;
    char            Padding[8];
};
typedef struct zs_order_s zs_order_t;


// todo
struct zs_quote_order_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];
    char            BranchID[BRANCH_ID_LEN];

    char            OrderID[ORDER_ID_LEN];
    char            OrderSysID[ORDER_SYSID_LEN];
};
typedef struct zs_quote_order_s zs_quote_order_t;


/* position data */
struct zs_position_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];

    uint64_t        Sid;
    ZSDirection     Direction;
    int32_t         Position;
    int32_t         TdPosition;
    int32_t         YdPosition;
    int32_t         YdFrozen;       // FIXME
    int32_t         Frozen;
    int32_t         Available;
    ZSPosDateType   PositionDate;

    int32_t         TradingDay;

    double          MarketValue;
    double          PositionCost;
    double          OpenCost;
    double          UseMargin;
    double          PositionPrice;
    double          LastPrice;
    double          PositionPnl;

    double          FrozenMargin;
    double          FrozenCommission;

    double          PriceTick;
    int32_t         Multiplier;
    int32_t         IsLast;
};
typedef struct zs_position_s zs_position_t;

/* position detail data */
struct zs_position_detail_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];
    char            TradeID[TRADE_ID_LEN];

    uint64_t        Sid;
    ZSDirection     Direction;
    int32_t         Volume;
    ZSPosDateType   PositionDate;

    int32_t         TradingDay;
    int32_t         OpenDate;

    double          OpenPrice;
    double          PositionPrice;
    double          UseMargin;
    double          PreSettlementPrice;
    double          CloseAmount;
    int32_t         CloseVolume;

    int32_t         Multiplier;
    int32_t         IsLast;
};
typedef struct zs_position_detail_s zs_position_detail_t;


/* trading account data */
struct zs_fund_account_s
{
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            AccountName[24];
    double          PreBalance;
    double          Balance;
    double          Available;
    double          FrozenCash;
    double          Commission;
    double          Margin;
    double          RealizedPnl;
    double          HoldingPnl;
    double          PositionValue;
    int64_t         UniqueID;
    int32_t         TradingDay;

    char            SHHolerCode[12];
    char            SZHolerCode[12];
};
typedef struct zs_fund_account_s zs_fund_account_t;


/* instrument info data */
struct zs_contract_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    char            SymbolName[ZS_SYMBOL_NAME_LEN];
    ZSExchangeID    ExchangeID;
    uint64_t        Sid;
    double          PriceTick;
    ZSProductClass  ProductClass;
    int32_t         Multiplier;
    int32_t         Decimal;
    int32_t         IsLast;

    ZSOptionType    OptionType;
    ZSStrikeType    StrikeType;
    double          StrikePrice;
    char            UnderlyingSymbol[8];
    uint64_t        UnderlyingSid;

    int32_t         ListDate;
    int32_t         ExpireDate;
    int32_t         DeliverDate;

    double          LongMarginRateByMoney;
    double          LongMarginRateByVolume;
    double          ShortMarginRateByMoney;
    double          ShortMarginRateByVolume;

    double          OpenRatioByMoney;
    double          OpenRatioByVolume;
    double          CloseRatioByMoney;
    double          CloseRatioByVolume;
    double          CloseTodayRatioByMoney;
    double          CloseTodayRatioByVolume;
};
typedef struct zs_contract_s zs_contract_t;


/* margin rate data */
struct zs_margin_rate_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    uint64_t        Sid;
    ZSRatioTypeType MarginRateType;

    double          LongMarginRateByMoney;
    double          LongMarginRateByVolume;
    double          ShortMarginRateByMoney;
    double          ShortMarginRateByVolume;
};
typedef struct zs_margin_rate_s zs_margin_rate_t;

struct zs_commission_rate_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    uint64_t        Sid;
    ZSRatioTypeType CommRateType;

    double          OpenRatioByMoney;
    double          OpenRatioByVolume;
    double          CloseRatioByMoney;
    double          CloseRatioByVolume;
    double          CloseTodayRatioByMoney;
    double          CloseTodayRatioByVolume;
};
typedef struct zs_commission_rate_s zs_commission_rate_t;


/* order request data */
struct zs_order_req_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];
    char            OrderID[ORDER_ID_LEN];

    uint64_t        Sid;
    double          OrderPrice;
    int32_t         OrderQty;
    ZSDirection     Direction;
    ZSOffsetFlag    OffsetFlag;
    ZSOrderType     OrderType;
    int64_t         OrderTime;

    // for api returned
    int32_t         FrontID;
    int32_t         SessionID;

    // IB
    ZSProductClass  ProductClass;
    char            Currency[4];

    void*           Contract;
};
typedef struct zs_order_req_s zs_order_req_t;


/* order cancel data */
struct zs_cancel_req_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            OrderID[ORDER_ID_LEN];

    int64_t         CancelTime;
    char            OrderSysID[ORDER_SYSID_LEN];

    int             FrontID;
    int             SessionID;
};
typedef struct zs_cancel_req_s zs_cancel_req_t;


/* quote order request data */
struct zs_quote_order_req_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];

    float           BidPrice;
    float           AskPrice;
    int32_t         BidQuantity;
    int32_t         AskQuantity;
    ZSOffsetFlag    Offset;
    ZSOrderType     OrderType;
    int64_t         OrderTime;
};
typedef struct zs_quote_order_req_s zs_quote_order_req_t;


/* subscribe request */
struct zs_subscribe_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    char            Exchange[8];
    // ZSExchangeID    ExchangeID;
    uint64_t        Sid;
};
typedef struct zs_subscribe_s zs_subscribe_t;


/* instrument confirm data */
struct zs_settle_confirm_s
{
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    int32_t         ConfirmDate;
    int32_t         ConfirmTime;
    int32_t         SettlementID;
    char        	CurrencyID[4];
};
typedef struct zs_settle_confirm_s zs_settle_confirm_t;


/* investor info data */
struct zs_investor_s
{
    char            BrokerID[BROKER_ID_LEN];
    char            AccountID[ACCOUNT_ID_LEN];
    char            AccountName[24];
    int32_t         OpenDate;
};
typedef struct zs_investor_s zs_investor_t;


/* for quote response for mm */
struct zs_forquote_rsp_s
{
    char                Symbol[ZS_SYMBOL_LEN];
    char                ForQuoteSysID[ORDER_SYSID_LEN]; //询价编号
    int32_t             ForQuoteTime;   //询价时间
    int32_t             TradingDay;
    int32_t             ActionDay;
    ZSExchangeID        ExchangeID;
};
typedef struct zs_forquote_rsp_s zs_forquote_rsp_t;


/* instrument status data */
struct zs_instrument_status_s
{
    ZSExchangeID        ExchangeID;
    char                Symbol[ZS_SYMBOL_LEN];
    ZSInstrumentStatus  InstrumentStatus;
    int32_t         	TradingSegmentSN;
    int32_t         	EnterTime;
    int32_t         	EnterReason;
};
typedef struct zs_instrument_status_s zs_instrument_status_t;


#pragma  pack(pop)



#ifdef __cplusplus
}
#endif

#endif//_ZS_API_STRUCT_H_

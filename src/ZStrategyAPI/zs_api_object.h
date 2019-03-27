/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the common api struct
 */

#ifndef _ZS_API_STRUCT_H_
#define _ZS_API_STRUCT_H_

#include <stdint.h>

#include "zs_common.h"

#define ORDER_SYSID_LEN     16
#define ACCOUNT_ID_LEN      16

#ifdef __cplusplus
extern "C" {
#endif


#pragma pack(push, 1)

struct zs_data_head_s 
{
    struct  zs_data_head_s* prev;
    struct  zs_data_head_s* next;
    void    (*Cleanup)(struct zs_data_head_s*);
    void*       CtxData;
    const char* pSymbol;            // point to the symbol id of the data body
    uint32_t    SymbolLength;
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


/* error data */
struct zs_error_data_s
{
    int             ErrorID;
    char            ErrorMsg[64];
};
typedef struct zs_error_data_s zs_error_data_t;

/* login */
struct zs_login_s
{
    char            AccountID[ACCOUNT_ID_LEN];
    int             Result;
};
typedef struct zs_login_s zs_login_t;

/* logout */
struct zs_logout_s
{
    char            AccountID[ACCOUNT_ID_LEN];
    int             Result;
};
typedef struct zs_logout_s zs_logout_t;

/* level1 market data */
struct zs_tick_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    int64_t         UpdateTime;
    uint64_t        Sid;
    int32_t         TradingDay;
    int32_t         ActionDay;

    double          OpenPrice;
    double          HighPrice;
    double          LowPrice;
    double          LastPrice;

    int64_t         Volume;
    int64_t         Turnover;
    double          OpenInterest;
    double          PreOpenInterest;
    double          PreClosePrice;
    double          SettlementPrice;

    double          UpperLimit;
    double          LowerLimit;
    double          PreDelta;
    double          CurrDelta;

    double          UpDown;
    int64_t         LastVolume;

    double          BidPrice[5];
    int32_t         BidVolume[5];

    float           AskPrice[5];
    int32_t         AskVolume[5];
};
typedef struct zs_tick_s zs_tick_t;


struct zs_l2_tick_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    int64_t         UpdateTime;
    uint64_t        Sid;
    int32_t         TradingDay;
    int32_t         ActionDay;

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
typedef struct zs_l2_tick_s zs_l2_tick_t;

struct zs_bar_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    int64_t         BarTime;            // 20181201093500
    uint64_t        Sid;

    int64_t         Volume;
    double          Amount;
    double          OpenInterest;       // for Future
    double          SettlePrice;        // for Future
    double          AdjustFactor;       // for Equity

    double          OpenPrice;
    double          HighPrice;
    double          LowPrice;
    double          ClosePrice;
};
typedef struct zs_bar_s zs_bar_t;


struct zs_trade_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];

    char            OrderSysID[ORDER_SYSID_LEN];
    char            TradeID[ORDER_SYSID_LEN];
    int64_t         OrderID;

    uint64_t        Sid;
    double          Price;
    int32_t         Volume;

    ZSDirectionType Direction;
    ZSOffsetFlag    Offset;
    int32_t         TradingDay;
    int64_t         TradeTime;

    double          RealizedPnl;
    double          CurrMargin;
    double          Commission;

    char            Padding[12];
};
typedef struct zs_trade_s zs_trade_t;

struct zs_order_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[8];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];

    uint64_t        Sid;
    double          Price;
    int32_t         Quantity;
    int32_t         Filled;
    ZSDirectionType Direction;
    ZSOffsetFlag    Offset;
    ZSOrderType     OrderType;
    ZSOrderStatus   Status;
    int64_t         OrderID;
    char            OrderSysID[ORDER_SYSID_LEN];
    int64_t         OrderTime;
    int64_t         CancelTime;
    int64_t         UpdateTime;

    // ctp 
    int32_t         FrontID;
    int32_t         SessionID;
    char            Padding[12];
};

typedef struct zs_order_s zs_order_t;

// todo
typedef struct zs_quote_order_s zs_quote_order_t;

struct zs_position_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[8];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];

    uint64_t        Sid;
    ZSDirectionType Direction;
    int32_t         Position;
    int32_t         TdPosition;
    int32_t         YdPosition;
    int32_t         Frozen;
    int32_t         PositionDate;

    double          PositionCost;
    double          UseMargin;
    double          AvgPrice;
    double          LastPrice;
    double          PositionPnl;

    int32_t         IsLast;
};
typedef struct zs_position_s zs_position_t;

struct zs_position_detail_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[8];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];

    uint64_t        Sid;
    ZSDirectionType Direction;
    int32_t         Position;
    int32_t         PositionDate;

    int32_t         TradingDay;
    int32_t         OpenDate;
    char            TradeID[16];

    double          OpenPrice;
    double          PositionPrice;
    double          UseMargin;
    double          PreSettlementPrice;
    double          CloseAmount;
    int32_t         CloseVolume;

    int32_t         IsLast;
};
typedef struct zs_position_detail_s zs_position_detail_t;

struct zs_fund_account_s
{
    char            BrokerID[8];
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

    char            SHHolerCode[12];
    char            SZHolerCode[12];
};
typedef struct zs_fund_account_s zs_fund_account_t;


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

    ZSOptionType    OptType;
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


struct zs_order_req_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            BrokerID[8];
    char            AccountID[ACCOUNT_ID_LEN];
    char            UserID[ACCOUNT_ID_LEN];

    uint64_t        Sid;
    double          Price;
    int32_t         Quantity;
    ZSDirectionType Direction;
    ZSOffsetFlag    Offset;
    ZSOrderType     OrderType;
    uint32_t        OrderID;
    int64_t         OrderTime;

    // IB
    ZSProductClass  ProductClass;
    char            Currency[4];

    void*           Contract;
};
typedef struct zs_order_req_s zs_order_req_t;

struct zs_cancel_req_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
    char            AccountID[ACCOUNT_ID_LEN];

    int64_t         OrderID;
    int64_t         CancelTime;
    char            OrderSysID[ORDER_SYSID_LEN];

    int             FrontID;
    int             SessionID;
};
typedef struct zs_cancel_req_s zs_cancel_req_t;


struct zs_quote_order_req_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    ZSExchangeID    ExchangeID;
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

struct zs_subscribe_s
{
    char            Symbol[ZS_SYMBOL_LEN];
    char            Exchange[8];
};
typedef struct zs_subscribe_s zs_subscribe_t;


#pragma  pack(pop)



#ifdef __cplusplus
}
#endif

#endif//_ZS_API_STRUCT_H_

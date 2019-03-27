/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 */

#pragma warning(disable:4819)

#ifndef _ZS_COMMON_H_INCLUDED_
#define _ZS_COMMON_H_INCLUDED_

/* ZS version */
#define ZS_Version          "0.0.1"

/* instrument length */
#define ZS_SYMBOL_LEN       32
#define ZS_SYMBOL_NAME_LEN  24


/* exchange name */
#define ZS_EXG_SSE          "SSE"
#define ZS_EXG_SZSE         "SZSE"
#define ZS_EXG_NEEQ         "NEEQ"
#define ZS_EXG_CFFEX        "CFFEX"
#define ZS_EXG_SHFE         "SHFE"
#define ZS_EXG_DCE          "DCE"
#define ZS_EXG_CZCE         "CZCE"
#define ZS_EXG_INE          "INE"
#define ZS_EXG_SGE          "SGE"
#define ZS_EXG_CFE          "CFE"
#define ZS_EXG_TEST         "TEST"

/* 市场ID */
typedef enum
{
    ZS_EI_Unkown,
    ZS_EI_SSE,
    ZS_EI_SZSE,
    ZS_EI_NEEQ,
    ZS_EI_CFFEX,
    ZS_EI_SHFE,
    ZS_EI_DCE,
    ZS_EI_CZCE,
    ZS_EI_INE,
    ZS_EI_SGE,
    ZS_EI_CFE,
    ZS_EI_TEST
}ZSExchangeID;


/* 数据类型 */
typedef enum 
{
    ZS_DT_Unkown,
    ZS_DT_MD_Bar,
    ZS_DT_MD_Tick,
    ZS_DT_MD_Level2,
    ZS_DT_MD_StepOrder,
    ZS_DT_MD_Transaction,
    ZS_DT_MD_ForQuote,
    ZS_DT_MD_Reserve,
    ZS_DT_Order,
    ZS_DT_Trade,
    ZS_DT_QuoteOrder,
    ZS_DT_QuoteTrade,
    ZS_DT_QryOrder,
    ZS_DT_QryTrade,
    ZS_DT_QryPositoin,
    ZS_DT_QryPositionDetail,
    ZS_DT_QryAccount
}ZSDataType;


/* 产品类型 */
typedef enum
{
    ZS_PC_Unkonwn,
    ZS_PC_Stock,
    ZS_PC_StockOption,
    ZS_PC_Future,
    ZS_PC_FutureOption
}ZSProductClass;

/* 期权类型 */
typedef enum 
{
    ZS_OPT_US,
    ZS_OPT_EU,
    ZS_OPT_Bermuda
}ZSOptionType;


/* 买卖方向 */
typedef enum 
{
    ZS_D_Long,
    ZS_D_Short
}ZSDirectionType;

/* 开平标志 */
typedef enum 
{
    ZS_OF_Open,
    ZS_OF_Close,
    ZS_OF_CloseToday,
    ZS_OF_CloseYd,
}ZSOffsetFlag;

/* 订单类型 */
typedef enum 
{
    ZS_OT_Limit,
    ZS_OT_Market,
    ZS_OT_StopLimit,
    ZS_OT_StopMarket
}ZSOrderType;

/* 订单状态 */
typedef enum 
{
    ZS_OS_NotSubmit,
    ZS_OS_Submiting,
    ZS_OS_Accepted,
    ZS_OS_PartFilled,
    ZS_OS_Filled,
    ZS_OS_Canceld,
    ZS_OS_PartCancled,
    ZS_OS_Rejected,
    ZS_OS_Triggered
}ZSOrderStatus;


/* 资产类型 */
typedef enum 
{
    ZS_A_Unknown,
    ZS_A_Equity,
    ZS_A_Future,
    ZS_A_Option
}ZSAssetType;

/* 数据频率 */
typedef enum 
{
    ZS_DF_Daily,
    ZS_DF_Hour,
    ZS_DF_Minute,
    ZS_DF_Tick
}ZSDataFrequency;

/* 运行模式 */
typedef enum 
{
    ZS_RM_Backtest,
    ZS_RM_Liverun
}ZSRunMode;

/* 价格类型 */
typedef enum 
{
    ZS_P_PreAdjusted,
    ZS_P_Real,
    ZS_P_PostAdjusted
}ZSPriceType;

/* 投机套保标志 */
typedef enum 
{
    ZS_HF_Speculation,
    ZS_HF_Arbitrage,
    ZS_HF_Hedge
}ZSHedgeFlag;

/* 合约状态 */
typedef enum
{
    ZS_IS_BeforeTrading,
    ZS_IS_NoTrading,
    ZS_IS_Continous,
    ZS_IS_AuctionOrdering,
    ZS_IS_AuctionBalance,
    ZS_IS_AuctionMatch,
    ZS_IS_Closed
}ZSInstrumentStatus;


#endif//_ZS_COMMON_H_INCLUDED_

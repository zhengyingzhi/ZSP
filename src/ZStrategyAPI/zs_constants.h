/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * ZStrategyAPI
 * define some constants defines
 */

#pragma warning(disable:4819)

#ifndef _ZS_CONSTANTS_H_INCLUDED_
#define _ZS_CONSTANTS_H_INCLUDED_


/* some commons */
#define ZS_BACKTEST_BROKERID    "0000"
#define ZS_BACKTEST_BROKERNAME  "INNER"
#define ZS_BACKTEST_ACCOUNTID   "backtest"
#define ZS_BACKTEST_ACCOUNTNAME "backtester"
#define ZS_BACKTEST_APINAME     "backtest_api"


/* instrument length */
#define ZS_SYMBOL_LEN       32
#define ZS_SYMBOL_NAME_LEN  24


/* exchange name */
#define ZS_EXCHANGE_UNKNOWN "UNKNOWN"
#define ZS_EXCHANGE_SSE     "SSE"
#define ZS_EXCHANGE_SZSE    "SZSE"
#define ZS_EXCHANGE_NEEQ    "NEEQ"
#define ZS_EXCHANGE_CFFEX   "CFFEX"
#define ZS_EXCHANGE_SHFE    "SHFE"
#define ZS_EXCHANGE_DCE     "DCE"
#define ZS_EXCHANGE_CZCE    "CZCE"
#define ZS_EXCHANGE_INE     "INE"
#define ZS_EXCHANGE_SGE     "SGE"
#define ZS_EXCHANGE_CFE     "CFE"
#define ZS_EXCHANGE_TEST    "TEST"


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


/* 数据事件类型 */
typedef enum 
{
    ZS_DT_Unkown,
    ZS_DT_Timer,
    ZS_DT_Other,
    ZS_DT_MD_Connected,
    ZS_DT_MD_Disconnected,
    ZS_DT_MD_Login,
    ZS_DT_MD_KLine,
    ZS_DT_MD_Tick,
    ZS_DT_MD_Tickl2,
    ZS_DT_MD_StepOrder,
    ZS_DT_MD_Transaction,
    ZS_DT_MD_ForQuote,
    ZS_DT_MD_Reserve,
    ZS_DT_Connected,
    ZS_DT_Disconnected,
    ZS_DT_Login,
    ZS_DT_Logout,
    ZS_DT_Auth,
    ZS_DT_Investor,
    ZS_DT_SettleConfirm,
    ZS_DT_QrySettle,
    ZS_DT_Order,
    ZS_DT_Trade,
    ZS_DT_QuoteOrder,
    ZS_DT_QuoteTrade,
    ZS_DT_QryOrder,
    ZS_DT_QryTrade,
    ZS_DT_QryPosition,
    ZS_DT_QryPositionDetail,
    ZS_DT_QryAccount,
    ZS_DT_QryContract,
    ZS_DT_QryMarginRate,
    ZS_DT_QryCommRate
}ZSDataType;


/* 产品类型 */
typedef enum
{
    ZS_PC_Unkonwn,
    ZS_PC_Stock,
    ZS_PC_StockOption,
    ZS_PC_Future,
    ZS_PC_FutureOption,
    ZS_PC_Bond,
    ZS_PC_Fund,
    ZS_PC_Other
}ZSProductClass;


/* 复权类型 */
typedef enum
{
    ZS_ADJ_None,
    ZS_ADJ_Pre,
    ZS_ADJ_Post
}ZSAdjustedType;


/* 期权类型 */
typedef enum
{
    ZS_OPT_None,
    ZS_OPT_Call,
    ZS_OPT_Put
}ZSOptionType;


/* 期权执行类型 */
typedef enum 
{
    ZS_SKT_None,
    ZS_SKT_US,
    ZS_SKT_EU,
    ZS_SKT_Bermuda
}ZSStrikeType;


/* 买卖方向 */
typedef enum 
{
    ZS_D_Unknown,
    ZS_D_Long,
    ZS_D_Short
}ZSDirection;

/* 开平标志 */
typedef enum 
{
    ZS_OF_Unkonwn,
    ZS_OF_Open,
    ZS_OF_Close,
    ZS_OF_CloseToday,
    ZS_OF_CloseYd
}ZSOffsetFlag;

/* 订单类型 */
typedef enum 
{
    ZS_OT_Limit,
    ZS_OT_Market,
    ZS_OT_StopLimit,
    ZS_OT_StopMarket
}ZSOrderType;

/* 订单状态
 * 注意与 finished_status_table 值对应
 */
typedef enum 
{
    ZS_OS_Unknown,
    ZS_OS_NotSubmit,
    ZS_OS_Submiting,
    ZS_OS_Pending,
    ZS_OS_PartFilled,
    ZS_OS_Filled,
    ZS_OS_Cancelld,
    ZS_OS_PartCanclled,
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

/* 运行状态 */
typedef enum
{
    ZS_RS_Unknown,
    ZS_RS_NotInit,
    ZS_RS_Inited,
    ZS_RS_Running,
    ZS_RS_Paused,
    ZS_RS_Stopping,
    ZS_RS_Stopped,
    ZS_RS_Expired
}ZSRunStatus;

/* 策略交易标志 */
typedef enum
{
    ZS_TF_Normal,           // 正常
    ZS_TF_Paused,           // 暂停交易
    ZS_TF_PauseOpen         // 暂停开仓
}ZSTradingFlag;


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


/* 持仓日期类型 */
typedef enum
{
    ZS_PD_Today,
    ZS_PD_Yesterday
}ZSPosDateType;


/* 费率类型 */
typedef enum
{
    ZS_RT_ByMoney,
    ZS_RT_ByVolume
}ZSRatioTypeType;


/* 策略类型 */
typedef enum
{
    ZS_ST_AssetSingle,
    ZS_ST_AssetMulti
}ZSStrategyTypeType;


/* C数据类型 */
typedef enum
{
    ZS_CT_Char,
    ZS_CT_String,
    ZS_CT_Short,
    ZS_CT_Int32,
    ZS_CT_Int64,
    ZS_CT_Float,
    ZS_CT_Double,
    ZS_CT_Pointer
}ZSCType;

#endif//_ZS_CONSTANTS_H_INCLUDED_

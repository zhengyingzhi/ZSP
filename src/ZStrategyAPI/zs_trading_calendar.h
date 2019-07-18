#ifndef _ZS_TRADING_CALENDAR_H_
#define _ZS_TRADING_CALENDAR_H_

#include <stdint.h>
#include <time.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_palloc.h>
#include <ZToolLib/ztl_times.h>

#include "zs_core.h"


struct zs_trading_calendar_s
{
    char        CalendarName[16];  // 交易日历名字
    char        MarketName[16];    // 交易日历名字
    char        StartDate[24];     // 开始时间
    char        EndDate[24];       // 结束时间
    int64_t     StartSession;      // 
    int64_t     EndSession;        // 
    int         IncludeWeekend;    // 是否包含周末
    ztl_pool_t* Pool;
    ztl_set_t*  Holidays;          // 节假日 value为 int64 时间
    ztl_array_t AllDays;           // 所有交易日
    ztl_array_t AllMinutes;        // 所有交易日的分钟
    ztl_array_t OpenClose;         // 各个交易日的开收盘时间
};


zs_trading_calendar_t* zs_tc_create(const char start_date[], const char end_date[], const ztl_array_t* holidays, int include_weekend);
zs_trading_calendar_t* zs_tc_create_by_market(const char start_date[], const char end_date[], const char* market_name);
zs_trading_calendar_t* zs_tc_create_by_tradingdays(const char start_date[], const char end_date[], ztl_array_t* trading_days);

int64_t zs_tc_release(zs_trading_calendar_t* tc);

int64_t zs_tc_prev_session(zs_trading_calendar_t* tc, int64_t session);
int64_t zs_tc_next_session(zs_trading_calendar_t* tc, int64_t session);

int64_t zs_tc_prev_minute(zs_trading_calendar_t* tc, int64_t minute_session);
int64_t zs_tc_next_minute(zs_trading_calendar_t* tc, int64_t minute_session);

int64_t zs_tc_minute_to_session(zs_trading_calendar_t* tc, int64_t minute_session);

bool zs_tc_is_session(zs_trading_calendar_t* tc, int64_t session);
bool zs_tc_is_holiday(zs_trading_calendar_t* tc, int64_t session);
bool zs_tc_is_open(zs_trading_calendar_t* tc, int64_t minute_session);

int32_t zs_tc_session_distance(zs_trading_calendar_t* tc, int64_t session1, int64_t session2);

int64_t zs_tc_session(zs_trading_calendar_t* tc, const char date[], int len);
int64_t zs_tc_minute_session(zs_trading_calendar_t* tc, const char datetime[], int len);

#define zs_tc_first_session(tc) (ztl_array_at(&(tc)->AllDays, 0))
#define zs_tc_last_session(tc)  (ztl_array_at(&(tc)->AllDays, ztl_array_size(&(tc)->AllDays) - 1))

void date_str_to_tm(struct tm* ptm, const char* date, int len);
void datetime_str_to_tm(struct tm* ptm, const char* datetime, int len);
time_t date_str_to_time(const char* date, int len);
time_t datetime_str_to_time(const char* datetime, int len);

int zs_date_range(ztl_array_t* dates, const char* start_date, const char* end_date);

#endif//_ZS_TRADING_CALENDAR_H_

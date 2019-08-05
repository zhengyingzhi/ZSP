#ifndef _ZS_TRADING_CALENDAR_H_
#define _ZS_TRADING_CALENDAR_H_

#include <stdint.h>
#include <time.h>

#include <ZToolLib/ztl_array.h>
#include <ZToolLib/ztl_map.h>
#include <ZToolLib/ztl_palloc.h>
#include <ZToolLib/ztl_times.h>

#include "zs_core.h"
#include "zs_times.h"


#define ZS_SESSION_INVALID      0

// the session value type
typedef int64_t zs_session_t;


// trading calendar object
struct zs_trading_calendar_s
{
    char        CalendarName[16];  // ������������
    char        MarketName[16];    // �г�����
    char        StartDate[24];     // ��ʼʱ��
    char        EndDate[24];       // ����ʱ��
    int64_t     StartSession;      // 
    int64_t     EndSession;        // 
    int         IncludeWeekend;    // �Ƿ������ĩ
    ztl_pool_t* Pool;
    ztl_set_t*  Holidays;          // �ڼ��� valueΪ session
    ztl_array_t AllDays;           // ���н�����
    ztl_array_t AllMinutes;        // ���н����յķ���
    ztl_array_t OpenClose;         // ���������յĿ�����ʱ��
};


/// create trading calendar object
zs_trading_calendar_t* zs_tc_create(const char start_date[], const char end_date[],
    const ztl_array_t* holidays, int include_weekend);
zs_trading_calendar_t* zs_tc_create_by_market(const char start_date[], const char end_date[],
    const char* market_name);
zs_trading_calendar_t* zs_tc_create_by_tradingdays(const char start_date[], const char end_date[],
    ztl_array_t* trading_days);

int zs_tc_release(zs_trading_calendar_t* tc);

zs_session_t zs_tc_prev_session(zs_trading_calendar_t* tc, zs_session_t session);
zs_session_t zs_tc_next_session(zs_trading_calendar_t* tc, zs_session_t session);

zs_session_t zs_tc_prev_minute(zs_trading_calendar_t* tc, zs_session_t minute_session);
zs_session_t zs_tc_next_minute(zs_trading_calendar_t* tc, zs_session_t minute_session);

zs_session_t zs_tc_minute_to_session(zs_trading_calendar_t* tc, zs_session_t minute_session);

bool zs_tc_is_session(zs_trading_calendar_t* tc, zs_session_t session);
bool zs_tc_is_holiday(zs_trading_calendar_t* tc, zs_session_t session);
bool zs_tc_is_open(zs_trading_calendar_t* tc, zs_session_t minute_session);

int32_t zs_tc_session_distance(zs_trading_calendar_t* tc, zs_session_t session1, zs_session_t session2);

zs_session_t zs_tc_to_session(zs_trading_calendar_t* tc, const char date[], int len);
zs_session_t zs_tc_to_minute_session(zs_trading_calendar_t* tc, const char datetime[], int len);

#define zs_tc_first_session(tc) *(zs_session_t*)(ztl_array_at(&(tc)->AllDays, 0))
#define zs_tc_last_session(tc)  *(zs_session_t*)(ztl_array_at(&(tc)->AllDays, ztl_array_size(&(tc)->AllDays) - 1))


// convert session to int date (20190102)
int32_t zs_session_to_date(zs_session_t session);

// get all dates between start and end
int zs_date_range(ztl_array_t* dates, const char* start_date, const char* end_date);


// all holidays from file, attention: elem is time_t
int zs_retrieve_holidays(ztl_array_t* holidays, const char* holiday_file);
int zs_retrieve_holidays2(ztl_set_t* holidays, const char* holiday_file);


#endif//_ZS_TRADING_CALENDAR_H_

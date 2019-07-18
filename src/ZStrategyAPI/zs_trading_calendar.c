#include <ZToolLib/ztl_utils.h>

#include "zs_trading_calendar.h"


#define SECONDS_PER_DAY     86400

#define _atoi_2(d) (((d)[0] - '0') * 10 + (d)[1] - '0')


typedef struct
{
    uint16_t beg_time;
    uint16_t end_time;
}time_section_t;

typedef struct
{
    uint16_t section_n;
    union section
    {
        uint32_t ti;
        time_section_t ts;
    };
    union section sections[4];
}market_time_t;

void date_str_to_tm(struct tm* ptm, const char* date, int len)
{
    if (len == 8) {
        ptm->tm_year = (int)atoi_n(date, 4) - 1900;
        ptm->tm_mon = _atoi_2(date + 4) - 1;
        ptm->tm_mday = _atoi_2(date + 6);
    }
    else {
        ptm->tm_year = (int)atoi_n(date, 4) - 1900;
        ptm->tm_mon = _atoi_2(date + 5) - 1;
        ptm->tm_mday = _atoi_2(date + 8);
    }
}

void datetime_str_to_tm(struct tm* ptm, const char* datetime, int len)
{
    if (len == 17) {
        ptm->tm_year = (int)atoi_n(datetime, 4) - 1900;
        ptm->tm_mon = _atoi_2(datetime + 4) - 1;
        ptm->tm_mday = _atoi_2(datetime + 6);
        ptm->tm_hour = _atoi_2(datetime + 9);
        ptm->tm_min = _atoi_2(datetime + 12);
        ptm->tm_sec = _atoi_2(datetime + 15);
    }
    else {
        ptm->tm_year = (int)atoi_n(datetime, 4) - 1900;
        ptm->tm_mon = _atoi_2(datetime + 5) - 1;
        ptm->tm_mday = _atoi_2(datetime + 8);
        ptm->tm_hour = _atoi_2(datetime + 11);
        ptm->tm_min = _atoi_2(datetime + 14);
        ptm->tm_sec = _atoi_2(datetime + 17);
    }
}

time_t date_str_to_time(const char* date, int len)
{
    struct tm ltm = { 0 };
    date_str_to_tm(&ltm, date, len);
    return mktime(&ltm);
}

time_t datetime_str_to_time(const char* datetime, int len)
{
    struct tm ltm = { 0 };
    datetime_str_to_tm(&ltm, datetime, len);
    return mktime(&ltm);
}

int zs_date_range(ztl_array_t* dates, const char* start_date, const char* end_date)
{
    int n;
    time_t t0, t1, tcur;
    t0 = date_str_to_time(start_date, (int)strlen(start_date));
    t1 = date_str_to_time(end_date, (int)strlen(end_date));
    tcur = t0;
    n = 0;

    while (tcur <= t1)
    {
        time_t* pt = ztl_array_push(dates);
        *pt = tcur;
        tcur += SECONDS_PER_DAY;
        n += 1;
    }
    return n;
}

int zs_is_session(ztl_array_t* sessions, time_t session)
{
    time_t* temp;
    int low, mid, high;
    low = 0;
    high = ztl_array_size(sessions);
    while (low <= high)
    {
        mid = (low + high) >> 1;
        temp = (time_t*)ztl_array_at(sessions, mid);
        if (session == *temp) {
            return mid;
        }
        else if (session < *temp) {
            high = mid - 1;
        }
        else {
            low = mid + 1;
        }
    }
    return -1;
}

int zs_search_sorted_index(ztl_array_t* sessions, time_t session)
{
    return -1;
}


zs_trading_calendar_t* zs_tc_create(const char start_date[], const char end_date[], const ztl_array_t* holidays, int include_weekend)
{
    ztl_pool_t* pool;
    zs_trading_calendar_t* tc;

    pool = ztl_create_pool(ZTL_DEFAULT_POOL_SIZE);

    tc = (zs_trading_calendar_t*)ztl_pcalloc(pool, sizeof(zs_trading_calendar_t));

    strncpy(tc->StartDate, start_date, sizeof(tc->StartDate) - 1);
    strncpy(tc->EndDate, end_date, sizeof(tc->EndDate) - 1);

    tc->IncludeWeekend = include_weekend;
    tc->Pool = pool;
    tc->Holidays = ztl_set_create(512);
    ztl_array_init(&tc->AllDays, pool, 4096, sizeof(time_t));

    for (uint32_t i = 0; i < ztl_array_size(holidays); ++i)
    {
        const char* holiday_date = (const char*)ztl_array_at(holidays, i);
        time_t t = date_str_to_time(holiday_date, (int)strlen(holiday_date));
        ztl_set_add(tc->Holidays, t);
    }

    tc->StartSession = zs_tc_session(tc, start_date, (int)strlen(start_date));
    tc->EndSession = zs_tc_session(tc, end_date, (int)strlen(end_date));

    return tc;
}

zs_trading_calendar_t* zs_tc_create_by_market(const char start_date[], const char end_date[], const char* market_name)
{
    return 0;
}

zs_trading_calendar_t* zs_tc_create_by_tradingdays(const char start_date[], const char end_date[], ztl_array_t* trading_days)
{
    return 0;
}

int64_t zs_tc_release(zs_trading_calendar_t* tc)
{
    if (tc)
    {
        ztl_set_release(tc->Holidays);
        ztl_destroy_pool(tc->Pool);
    }
    return 0;
}

int64_t zs_tc_prev_session(zs_trading_calendar_t* tc, int64_t session)
{
    int64_t next, first;

    if (ztl_array_size(&tc->AllDays) < 1) {
        return 0;
    }

    next = session;
    first = *(int64_t*)zs_tc_first_session(tc);

    if (next > *(int64_t*)zs_tc_last_session(tc)) {
        return 0;
    }

    do {
        next -= SECONDS_PER_DAY;
        if (next < first) {
            break;
        }

        if (zs_is_session(&tc->AllDays, next)) {
            return next;
        }
    } while (true);
    return 0;
}

int64_t zs_tc_next_session(zs_trading_calendar_t* tc, int64_t session)
{
    int64_t next, last;

    if (ztl_array_size(&tc->AllDays) < 1) {
        return 0;
    }

    next = session;
    last = *(int64_t*)zs_tc_last_session(tc);

    if (next < *(int64_t*)zs_tc_first_session(tc)) {
        return 0;
    }

    do {
        next += SECONDS_PER_DAY;
        if (next > last) {
            break;
        }

        if (zs_is_session(&tc->AllDays, next)) {
            return next;
        }
    } while (true);
    return 0;
}

int64_t zs_tc_prev_minute(zs_trading_calendar_t* tc, int64_t minute_session)
{
    return 0;
}

int64_t zs_tc_next_minute(zs_trading_calendar_t* tc, int64_t minute_session)
{
    return 0;
}

int64_t zs_tc_minute_to_session(zs_trading_calendar_t* tc, int64_t minute_session)
{
    time_t session, min_sess;
    struct tm ltm = { 0 };

    min_sess = (time_t)minute_session;
    LOCALTIME_S(&ltm, &min_sess);
    ltm.tm_hour = 0;
    ltm.tm_min = 0;
    ltm.tm_sec = 0;

    session = mktime(&ltm);

    if (zs_tc_is_session(tc, session)) {
        return session;
    }
    return 0;
}

bool zs_tc_is_session(zs_trading_calendar_t* tc, int64_t session)
{
    if (zs_is_session(&tc->AllDays, (time_t)session)) {
        return true;
    }
    return false;
}

bool zs_tc_is_holiday(zs_trading_calendar_t* tc, int64_t session)
{
    if (ztl_set_count(tc->Holidays, session)) {
        return true;
    }
    return false;
}

bool zs_tc_is_open(zs_trading_calendar_t* tc, int64_t minute_session)
{
    return false;
}

int32_t zs_tc_session_distance(zs_trading_calendar_t* tc, int64_t session1, int64_t session2)
{
    return 0;
}

int64_t zs_tc_session(zs_trading_calendar_t* tc, const char date[], int len)
{
    time_t t;
    t = date_str_to_time(date, len);
    if (zs_tc_is_session(tc, t)) {
        return t;
    }
    return 0;
}

int64_t zs_tc_minute_session(zs_trading_calendar_t* tc, const char datetime[], int len)
{
    return 0;
}

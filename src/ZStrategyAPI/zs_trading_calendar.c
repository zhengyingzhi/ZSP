#include <ZToolLib/ztl_times.h>
#include <ZToolLib/ztl_utils.h>

#include "zs_error.h"
#include "zs_trading_calendar.h"


#define ZS_HOLIDAY_DEFAULT_NUM      1024

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


static inline zs_session_t zs_make_session(struct tm* ptm)
{
    return mktime(ptm);
}

int32_t zs_session_to_date(zs_session_t session)
{
    struct tm ltm = { 0 };
    LOCALTIME_S(&ltm, &session);
    return (ltm.tm_year + 1900) * 10000 + (ltm.tm_mon + 1) * 100 + ltm.tm_mday;
}

int zs_date_range(ztl_array_t* dates, const char* start_date, const char* end_date)
{
    int n;
    time_t t0, t1, tcur;
    t0 = zs_date_str_to_time(start_date, (int)strlen(start_date));
    t1 = zs_date_str_to_time(end_date, (int)strlen(end_date));
    tcur = t0;
    n = 0;

    while (tcur <= t1)
    {
        ztl_array_push_back(dates, &tcur);
        tcur += ZS_SECONDS_PER_DAY;
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


zs_trading_calendar_t* zs_tc_create(
    const char start_date[], const char end_date[],
    const ztl_array_t* holidays, int include_weekend)
{
    time_t* pt;
    ztl_pool_t* pool;
    zs_trading_calendar_t* tc;

    pool = ztl_create_pool(4096);

    tc = (zs_trading_calendar_t*)ztl_pcalloc(pool, sizeof(zs_trading_calendar_t));

    strncpy(tc->StartDate, start_date, sizeof(tc->StartDate) - 1);
    strncpy(tc->EndDate, end_date, sizeof(tc->EndDate) - 1);

    tc->IncludeWeekend = include_weekend;
    tc->Pool = pool;
    tc->Holidays = ztl_set_create(ZS_HOLIDAY_DEFAULT_NUM);
    ztl_array_init(&tc->AllDays, NULL, 4096, sizeof(int64_t));

    for (uint32_t i = 0; i < ztl_array_size(holidays); ++i)
    {
        pt = (time_t*)ztl_array_at(holidays, i);
        ztl_set_add(tc->Holidays, *pt);
    }

    tc->StartSession = zs_tc_to_session(tc, start_date, (int)strlen(start_date));
    tc->EndSession = zs_tc_to_session(tc, end_date, (int)strlen(end_date));

    return tc;
}

zs_trading_calendar_t* zs_tc_create_by_market(const char start_date[], const char end_date[], const char* market_name)
{
    zs_trading_calendar_t* tc;

    ztl_array_t holidays;
    ztl_array_init(&holidays, NULL, ZS_HOLIDAY_DEFAULT_NUM, sizeof(int64_t));
    zs_retrieve_holidays(&holidays, "holidays.txt");

    tc = zs_tc_create(start_date, end_date, &holidays, 0);

    ztl_array_release(&holidays);

    return tc;
}

zs_trading_calendar_t* zs_tc_create_by_tradingdays(const char start_date[], const char end_date[], ztl_array_t* trading_days)
{
    return 0;
}

int zs_tc_release(zs_trading_calendar_t* tc)
{
    if (tc)
    {
        ztl_set_release(tc->Holidays);
        ztl_array_release(&tc->AllDays);
        // ztl_array_release(&tc->AllMinutes);
        // ztl_array_release(&tc->OpenClose);
        ztl_destroy_pool(tc->Pool);
    }
    return ZS_OK;
}

zs_session_t zs_tc_prev_session(zs_trading_calendar_t* tc, zs_session_t session)
{
    zs_session_t prev, first;

    if (ztl_array_size(&tc->AllDays) < 1) {
        return ZS_SESSION_INVALID;
    }

    prev = session;
    first = *(zs_session_t*)zs_tc_first_session(tc);

    if (prev > *(zs_session_t*)zs_tc_last_session(tc)) {
        return ZS_SESSION_INVALID;
    }

    do {
        prev -= ZS_SECONDS_PER_DAY;
        if (prev < first) {
            break;
        }

        if (zs_is_session(&tc->AllDays, prev)) {
            return prev;
        }
    } while (true);
    return ZS_SESSION_INVALID;
}

zs_session_t zs_tc_next_session(zs_trading_calendar_t* tc, zs_session_t session)
{
    zs_session_t next, last;

    if (ztl_array_size(&tc->AllDays) < 1) {
        return ZS_SESSION_INVALID;
    }

    next = session;
    last = *(zs_session_t*)zs_tc_last_session(tc);

    if (next < *(zs_session_t*)zs_tc_first_session(tc)) {
        return ZS_SESSION_INVALID;
    }

    do {
        next += ZS_SECONDS_PER_DAY;
        if (next > last) {
            break;
        }

        if (zs_is_session(&tc->AllDays, next)) {
            return next;
        }
    } while (true);
    return ZS_SESSION_INVALID;
}

zs_session_t zs_tc_prev_minute(zs_trading_calendar_t* tc, zs_session_t minute_session)
{
    zs_session_t session;
    minute_session -= 60;

    session = zs_tc_minute_to_session(tc, minute_session);
    if (zs_is_session(&tc->AllDays, minute_session)) {
        return session;
    }
    return ZS_SESSION_INVALID;
}

zs_session_t zs_tc_next_minute(zs_trading_calendar_t* tc, zs_session_t minute_session)
{
    zs_session_t session;
    minute_session += 60;

    session = zs_tc_minute_to_session(tc, minute_session);
    if (zs_is_session(&tc->AllDays, minute_session)) {
        return session;
    }
    return ZS_SESSION_INVALID;
}

zs_session_t zs_tc_minute_to_session(zs_trading_calendar_t* tc, zs_session_t minute_session)
{
    time_t session, min_sess;
    struct tm ltm = { 0 };

    min_sess = (time_t)minute_session;
    LOCALTIME_S(&ltm, &min_sess);
    ltm.tm_hour = 0;
    ltm.tm_min = 0;
    ltm.tm_sec = 0;

    session = zs_make_session(&ltm);

    if (zs_tc_is_session(tc, session)) {
        return session;
    }
    return ZS_SESSION_INVALID;
}

bool zs_tc_is_session(zs_trading_calendar_t* tc, zs_session_t session)
{
    if (zs_is_session(&tc->AllDays, (time_t)session)) {
        return true;
    }
    return false;
}

bool zs_tc_is_holiday(zs_trading_calendar_t* tc, zs_session_t session)
{
    if (ztl_set_count(tc->Holidays, session)) {
        return true;
    }
    return false;
}

bool zs_tc_is_open(zs_trading_calendar_t* tc, zs_session_t minute_session)
{
    return false;
}

int32_t zs_tc_session_distance(zs_trading_calendar_t* tc,
    zs_session_t session1, zs_session_t session2)
{
    return 0;
}

zs_session_t zs_tc_to_session(zs_trading_calendar_t* tc, const char date[], int len)
{
    time_t t;
    t = zs_date_str_to_time(date, len);
    if (t && zs_tc_is_session(tc, t)) {
        return t;
    }
    return ZS_SESSION_INVALID;
}

zs_session_t zs_tc_to_minute_session(zs_trading_calendar_t* tc, const char datetime[], int len)
{
    struct tm ltm = { 0 };
    if (zs_datetime_str_to_tm(&ltm, datetime, len) != ZS_OK) {
        return 0;
    }

    ltm.tm_hour = 0;
    ltm.tm_min = 0;
    ltm.tm_sec = 0;
    return zs_make_session(&ltm);
}


int zs_retrieve_holidays(ztl_array_t* holidays, const char* holiday_file)
{
    char    buffer[4000] = "";
    char*   items[1024] = { 0 };
    int     size, count;
    time_t  t;

    size = read_file_content(holiday_file, buffer, sizeof(buffer) - 1);
    if (size <= 0) {
        return 0;
    }

    count = str_delimiter(buffer, items, 1024, ',');
    for (int i = 0; i < count; ++i)
    {
        char* s = items[i];
        lefttrim(s);
        righttrim(s);

        t = zs_date_str_to_time(s, (int)strlen(s));
        if (t == 0) {
            continue;
        }

        ztl_array_push_back(holidays, &t);
    }

    return ztl_array_size(holidays);
}

int zs_retrieve_holidays2(ztl_set_t* holidays, const char* holiday_file)
{
    ztl_array_t array;
    ztl_array_init(&array, NULL, 1024, sizeof(time_t));
    zs_retrieve_holidays(&array, holiday_file);

    for (uint32_t i = 0; i < ztl_array_size(&array); ++i)
    {
        time_t* pt = (time_t*)ztl_array_at(&array, i);
        ztl_set_add(holidays, *pt);
    }

    ztl_array_release(&array);
    return ztl_set_size(holidays);
}

#include <ZToolLib/ztl_times.h>
#include <ZToolLib/ztl_utils.h>

#include "zs_error.h"
#include "zs_times.h"



void zs_date_int_to_tm(struct tm* ptm, int32_t date)
{
    // 20190102
    ptm->tm_year = date / 10000 - 1900;
    ptm->tm_mon  = (date / 100 % 100) - 1;
    ptm->tm_mday = date % 10000;
}

int zs_date_str_to_tm(struct tm* ptm, const char* date, int len)
{
    if (len == 8)
    {
        // 20190102
        ptm->tm_year = (int)atoi_n(date, 4) - 1900;
        ptm->tm_mon  = _atoi_2(date + 4) - 1;
        ptm->tm_mday = _atoi_2(date + 6);
        return ZS_OK;
    }
    else if (len == 10)
    {
        // 2019-01-02
        ptm->tm_year = (int)atoi_n(date, 4) - 1900;
        ptm->tm_mon  = _atoi_2(date + 5) - 1;
        ptm->tm_mday = _atoi_2(date + 8);
        return ZS_OK;
    }
    return ZS_ERROR;
}

int zs_datetime_str_to_tm(struct tm* ptm, const char* datetime, int len)
{
    if (len == 17)
    {
        // 20190102 10:01:02
        ptm->tm_year = (int)atoi_n(datetime, 4) - 1900;
        ptm->tm_mon  = _atoi_2(datetime + 4) - 1;
        ptm->tm_mday = _atoi_2(datetime + 6);
        ptm->tm_hour = _atoi_2(datetime + 9);
        ptm->tm_min  = _atoi_2(datetime + 12);
        ptm->tm_sec  = _atoi_2(datetime + 15);
        return ZS_OK;
    }
    else if (len == 19)
    {
        // 2019-01-02 10:01:02
        ptm->tm_year = (int)atoi_n(datetime, 4) - 1900;
        ptm->tm_mon  = _atoi_2(datetime + 5) - 1;
        ptm->tm_mday = _atoi_2(datetime + 8);
        ptm->tm_hour = _atoi_2(datetime + 11);
        ptm->tm_min  = _atoi_2(datetime + 14);
        ptm->tm_sec  = _atoi_2(datetime + 17);
        return ZS_OK;
    }
    return ZS_ERROR;
}


time_t zs_date_int_to_time(int32_t date)
{
    struct tm ltm = { 0 };
    zs_date_int_to_tm(&ltm, date);
    return mktime(&ltm);
}

time_t zs_date_str_to_time(const char* date, int len)
{
    struct tm ltm = { 0 };
    if (zs_date_str_to_tm(&ltm, date, len) != ZS_OK) {
        return 0;
    }
    return mktime(&ltm);
}

time_t zs_datetime_str_to_time(const char* datetime, int len)
{
    struct tm ltm = { 0 };
    if (zs_datetime_str_to_tm(&ltm, datetime, len) != ZS_OK) {
        return 0;
    }
    return mktime(&ltm);
}

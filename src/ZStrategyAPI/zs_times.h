#ifndef _ZS_TIMES_H_INCLUDED_
#define _ZS_TIMES_H_INCLUDED_

#include <stdint.h>
#include <time.h>

#include <ZToolLib/ztl_times.h>

// seconds per day
#define ZS_SECONDS_PER_DAY      86400

// convert two chars as integer
#define _atoi_2(d) (((d)[0] - '0') * 10 + (d)[1] - '0')


// 20190102 -->> tm
void zs_date_int_to_tm(struct tm* ptm, int32_t date);

// "2019-01-02" or "20190102" -->> tm
int zs_date_str_to_tm(struct tm* ptm, const char* date, int len);

// "2019-01-02 10:01:02" or "20190102 10:01:02" -->> tm
int zs_datetime_str_to_tm(struct tm* ptm, const char* datetime, int len);


// 20190102 -->> time_t
time_t zs_date_int_to_time(int32_t date);

// "2019-01-02" or "20190102" -->> time_t
time_t zs_date_str_to_time(const char* date, int len);

// "2019-01-02 10:01:02" or "20190102 10:01:02" -->> time_t
time_t zs_datetime_str_to_time(const char* datetime, int len);


#endif//_ZS_TIMES_H_INCLUDED_

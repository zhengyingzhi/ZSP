#include <stdio.h>
#include <assert.h>

#include <ZToolLib/ztl_utils.h>

#include <ZStrategyAPI/zs_api_object.h>
#include <ZStrategyAPI/zs_assets.h>
#include <ZStrategyAPI/zs_trading_calendar.h>

void test_asset_finder();
void test_calendar();
void test_array();

int main(int argc, char* argv[])
{
    fprintf(stderr, "hello ZStrategyAPI.Test\n");
#if defined (_WIN64) || defined(__x86_64__)
    fprintf(stderr, "x64\n");
#endif

    // test_asset_finder();
    // test_calendar();
    test_array();
    return 0;
}

void test_asset_finder()
{
    int rv;
    zs_asset_finder_t* asset_finder;
    asset_finder = zs_asset_create(NULL, NULL, 0);

    zs_sid_t sid = 0;
    zs_contract_t contract1;
    memset(&contract1, 0, sizeof(contract1));
    strcpy(contract1.Symbol, "rb1905");
    rv = zs_asset_add(asset_finder, &sid, "rb1905", 6, &contract1);
    contract1.Sid = sid;
    assert(rv == 0);

    zs_contract_t contract2;
    memset(&contract2, 0, sizeof(contract2));
    strcpy(contract2.Symbol, "600111.SH");
    rv = zs_asset_add(asset_finder, &sid, "600111.SH", 9, &contract2);
    contract2.Sid = sid;
    assert(rv == 0);

    zs_sid_t s1 = zs_asset_lookup(asset_finder, "rb1905", 6);
    zs_sid_t s2 = zs_asset_lookup(asset_finder, "600111.SH", 9);
    zs_sid_t s3 = zs_asset_lookup(asset_finder, "600111", 6);

    zs_contract_t* pcont1 = zs_asset_find_by_sid(asset_finder, s1);
    zs_contract_t* pcont2 = zs_asset_find_by_sid(asset_finder, s2);
    zs_contract_t* pcont3 = zs_asset_find_by_sid(asset_finder, s3);
    assert(pcont1);
    assert(pcont2);
    assert(!pcont3);

    rv = zs_asset_del(asset_finder, "rb1905", 6);
    assert(rv == 0);
    pcont1 = zs_asset_find(asset_finder, "rb1905", 6);
    assert(!pcont1);
    pcont2 = zs_asset_find(asset_finder, "600111.SH", 9);
    assert(s2);

    // test multiple
    int n = 0;
    const int count = 10000;
    zs_sid_t sid_arr[10000] = { 0 };
    for (int i = 0; i < count; ++i)
    {
        char buf[32] = "";
        int len = 0;
        if (i < count / 2)
            len = sprintf(buf, "%06d.SZ", i + 1);
        else
            len = sprintf(buf, "6%05d.SH", i);

        zs_sid_t sid = 0;
        zs_contract_t contract;
        memset(&contract, 0, sizeof(contract1));
        strncpy(contract.Symbol, buf, len);
        rv = zs_asset_add_copy(asset_finder, &sid, buf, len, &contract, sizeof(contract));

        zs_contract_t* pcont = zs_asset_find_by_sid(asset_finder, sid);
        pcont->Sid = sid;
        sid_arr[i] = sid;
    }
    int64_t t0 = query_tick_count();
    for (int i = 0; i < count; ++i)
    {
        zs_sid_t sid = sid_arr[i];
        zs_contract_t* pcont = zs_asset_find_by_sid(asset_finder, sid);
        assert(pcont);

        char buf[32] = "";
        int len = 0;
        if (i < count / 2)
            len = sprintf(buf, "%06d.SZ", i + 1);
        else
            len = sprintf(buf, "6%05d.SH", i);
        assert(strcmp(buf, pcont->Symbol) == 0);
        assert(pcont->Sid == sid);

        n += 1;
    }
    int64_t t1 = query_tick_count();
    int32_t cost = tick_to_us(t0, t1);

    zs_asset_release(asset_finder);
    fprintf(stderr, "assets tested %d, cost:%dus -->> %.3lfus/1\n", n, cost, cost / (double)n);
}

void test_calendar()
{
    struct tm ltm1 = { 0 };
    struct tm ltm2 = { 0 };
    struct tm ltm3 = { 0 };
    struct tm ltm4 = { 0 };
    char date1[] = "2019-03-20";
    char date2[] = "20190320";
    char datetime1[] = "2019-03-20 10:12:13";
    char datetime2[] = "20190320 10:12:14";
    date_str_to_tm(&ltm1, date1, strlen(date1));
    date_str_to_tm(&ltm2, date2, strlen(date2));
    datetime_str_to_tm(&ltm3, datetime1, strlen(datetime1));
    datetime_str_to_tm(&ltm4, datetime2, strlen(datetime2));

    ztl_array_t larr;
    ztl_array_init(&larr, NULL, 16, sizeof(time_t));
    int n = zs_date_range(&larr, "2019-02-01", "2019-03-01");
    fprintf(stdout, "n=%d\n", n);
    for (int i = 0; i < n; ++i)
    {
        time_t* tcur = (time_t*)ztl_array_at(&larr, i);
        char buf[16] = "";
        struct tm ltm = { 0 };
        LOCALTIME_S(&ltm, tcur);
        strftime(buf, sizeof(buf) - 1, "%Y-%m-%d", &ltm);
        if ((ltm.tm_wday == 6 || ltm.tm_wday == 0)) {
            // weekend
            fprintf(stdout, "#weekend %s, weekday=%d\n", buf, ltm.tm_wday);
        }
//         else if (is_holiday(tcur)) {
//             fprintf(stdout, "#holiday %s, weekday=%d\n", buf, ltm.tm_wday);
//             continue;
//         }
    }
}


typedef struct c_vector_s c_vector_t;
struct c_vector_s
{
    void*   array;
    int     eltsize;
    int     size;
    int     capacity;
    void (*init)(c_vector_t* vec, int eltsize, int init_size);
    void (*resize)(c_vector_t* vec);
    void (*push_char)(c_vector_t* vec, int8_t val);
    void (*push_short)(c_vector_t* vec, int64_t val);
    void (*push_int)(c_vector_t* vec, int32_t val);
    void (*push_int64)(c_vector_t* vec, int64_t val);
    void (*push_float)(c_vector_t* vec, float val);
    void (*push_double)(c_vector_t* vec, double val);
};

void c_vector_init(c_vector_t* vec, int eltsize, int init_size)
{
    vec->eltsize = eltsize;
    vec->size = 0;
    vec->capacity = ztl_align(init_size, 4);
    vec->array = malloc(eltsize * vec->capacity);
}

void c_vector_resize(c_vector_t* vec)
{
    vec->capacity <<= 1;
    vec->array = realloc(vec->array, vec->capacity * vec->eltsize);
}

void c_vector_push_int(c_vector_t* vec, int val)
{
    if (vec->size == vec->capacity) {
        c_vector_resize(vec);
    }
    int* pv = (int*)vec->array;
    pv[vec->size++] = val;
}

void test_array()
{
    c_vector_t vec;
    c_vector_init(&vec, sizeof(int), 3);

    int* pv;

    int count = 10;
    for (int i = 0; i < count; ++i)
    {
        pv = vec.array;
        c_vector_push_int(&vec, i);
        // fprintf(stdout, "\n");
    }

    pv = vec.array;
    for (int i = 0; i < vec.size; ++i)
    {
        fprintf(stdout, "%d ", pv[i]);
    }
    fprintf(stdout, "\n");

}

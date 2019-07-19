#include <stdio.h>
#include <assert.h>
#include <float.h>
#include <locale.h>

#include <ZToolLib/ztl_utils.h>

#include <ZStrategyAPI/zs_api_object.h>
#include <ZStrategyAPI/zs_assets.h>
#include <ZStrategyAPI/zs_trading_calendar.h>
#include <ZStrategyAPI/zs_broker_info.h>
#include <ZStrategyAPI/zs_bar_generator.h>

void test_asset_finder();
void test_calendar();
void test_array();
void test_broker_info();

// c data frame
#include <ZStrategyAPI/zs_hashdict.h>
typedef enum
{
    ZS_VT_Unkown,
    ZS_VT_Pointer,
    ZS_VT_String,
    ZS_VT_Double,
    ZS_VT_INT64,
    ZS_VT_INT32,
    ZS_VT_INT16,
    ZS_VT_INT8
}ZSValueType;

typedef struct
{
    const char*     Name;           // the field name
    ztl_array_t*    pIndexs;        // the index
    ztl_array_t     Array;          // array data
    // uint32_t        NameInt;        // 
    uint32_t        StartPos;
    uint32_t        Count;
    ZSValueType     Type;
}zs_series_t;

typedef struct  
{
    ztl_pool_t*     Pool;
    ztl_array_t     Indexs;             // the index
    ztl_array_t     SeriesArray;        // keep as array for easiy visit
    ztl_dict_t*     SeriesDict;         // <field_name, series_object>
    uint32_t        ColNum;             // 列数 == len(SeriesDict)
    uint32_t        RowNum;             // 行数 == len(Indexs)
}zs_data_frame_t;

/*
zs_data_frame_t 保存二维数据
1. 一个列索引：可能是序号，也可以是int64的时间戳，指定索引即可取到该某series上该索引上的数据
2. 要取某行的数据时，使用该行的索引，依次遍历SeriesArray，对各series取该索引上的值即可
 */
zs_series_t* zs_find_series(ztl_dict_t* dict, const char* name, int length)
{
    zs_series_t* series = NULL;
    ZStrKey key = { length, (char*)name };
    dictEntry* entry = dictFind(dict, &key);
    if (entry) {
        series = (zs_series_t*)entry->v.val;
    }
    return series;
}

double zs_get_value(ztl_dict_t* dict, const char* name, int index)
{
    zs_series_t* series;

    series = zs_find_series(dict, name, (int)strlen(name));
    if (!series)
    {
        fprintf(stderr, "not find series for %s\n", name);
        return DBL_MAX;
    }

    return *(double*)ztl_array_at(&series->Array, index);
}

void zs_data_demo()
{
    ztl_pool_t* pool;
    pool = ztl_create_pool(ZTL_DEFAULT_POOL_SIZE);

    int row = 4;
    int col = 3;        // symbol, open, close

    zs_data_frame_t df;
    df.Pool = pool;
    ztl_array_init(&df.Indexs, NULL, row, sizeof(uint32_t));
    ztl_array_init(&df.SeriesArray, NULL, col, sizeof(zs_series_t*));
    df.SeriesDict = dictCreate(&strHashDictType, &df);

    // make index
    for (int i = 0; i < row; ++i)
    {
        uint32_t* dst = (uint32_t*)ztl_array_push(&df.Indexs);
        *dst = i;
    }

    // make series
    typedef double value_t;
    const char* fields[] = { "symbol", "open", "close", NULL };
    value_t prices[4][2] = { { 10, 11 }, { 10.2, 11.2 }, { 10.4, 11.4 }, { 10.6, 11.6 } };

    zs_series_t* series;
    for (int i = 0; fields[i]; ++i)
    {
        ZStrKey* key;
        // series = (zs_series_t*)ztl_pcalloc(pool, sizeof(series));
        series = (zs_series_t*)malloc(sizeof(series));
        series->Name = fields[i];
        series->pIndexs = &df.Indexs;
        series->StartPos = 0;
        series->Count = 0;
        ztl_array_init(&series->Array, NULL, row, sizeof(value_t));
        ztl_array_push_back(&df.SeriesArray, series);

        key = ztl_palloc(pool, ztl_align(sizeof(ZStrKey), sizeof(void*)));
        key->len = (int)strlen(fields[i]);
        key->ptr = (char*)fields[i];
        dictAdd(df.SeriesDict, key, series);

        if (i > 0)
        {
            value_t* dst;
#if 0
            dst = ztl_array_push(&series->Array);
            *dst = prices[i - 1][0];
            fprintf(stderr, "push to %s, value:%lf\n", series->Name, *dst);
            dst = ztl_array_push(&series->Array);
            *dst = prices[i - 1][1];
            fprintf(stderr, "push to %s, value:%lf\n", series->Name, *dst);
#else
            dst = ztl_array_push_n(&series->Array, 2);
            memcpy(dst, prices[i], sizeof(value_t) * 2);
#endif
        }
    }

    // 若要取数据index+field: 找到series，取index即可
    // 若要取某字段的序列数据，根据field从SeriesDict中查找，则返回一个 ptr+size即可？
    // 可以取某个区间的序列数据，修改 series 的 pos 和 count 即可 
    for (int i = 0; fields[i]; ++i)
    {
        series = zs_find_series(df.SeriesDict, fields[i], (int)strlen(fields[i]));
        if (!series)
        {
            fprintf(stderr, "not find series for %s\n", fields[i]);
            continue;
        }

        if (i == 0)
            continue;

        for (int k = 0; k < (int)ztl_array_size(&series->Array); ++k)
        {
            value_t px = *(value_t*)ztl_array_at(&series->Array, k);
            fprintf(stderr, "%s px:%lf\n", series->Name, px);
        }
    }

    // get one value
    value_t value1, value2;
    value1 = zs_get_value(df.SeriesDict, "open", 0);
    value2 = zs_get_value(df.SeriesDict, "close", 1);
    fprintf(stderr, "value1:%lf, value2:%lf\n", value1, value2);
}


int main(int argc, char* argv[])
{
    fprintf(stderr, "hello ZStrategyAPI.Test\n");
#if defined (_WIN64) || defined(__x86_64__)
    fprintf(stderr, "x64\n");
#endif

    // test_asset_finder();
    // test_calendar();
    // test_array();
    // test_broker_info();
    zs_data_demo();
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
    contract1.ExchangeID = ZS_EI_SHFE;
    rv = zs_asset_add(asset_finder, &sid, contract1.ExchangeID, contract1.Symbol, 6, &contract1);
    contract1.Sid = sid;
    assert(rv == 0);

    zs_contract_t contract2;
    memset(&contract2, 0, sizeof(contract2));
    strcpy(contract2.Symbol, "000001");
    contract2.ExchangeID = ZS_EI_SSE;
    rv = zs_asset_add(asset_finder, &sid, contract2.ExchangeID, contract2.Symbol, 6, &contract2);
    contract2.Sid = sid;
    assert(rv == 0);

    zs_contract_t contract3;
    memset(&contract3, 0, sizeof(contract3));
    strcpy(contract3.Symbol, "000001");
    contract3.ExchangeID = ZS_EI_SZSE;
    rv = zs_asset_add(asset_finder, &sid, contract3.ExchangeID, contract3.Symbol, 6, &contract3);
    contract2.Sid = sid;
    assert(rv == 0);

    zs_sid_t s1 = zs_asset_lookup(asset_finder, ZS_EI_SHFE, "rb1905", 6);
    zs_sid_t s2 = zs_asset_lookup(asset_finder, ZS_EI_SSE, "000001", 6);
    zs_sid_t s3 = zs_asset_lookup(asset_finder, ZS_EI_SZSE, "000001", 6);

    zs_contract_t* pcont1 = zs_asset_find_by_sid(asset_finder, s1);
    zs_contract_t* pcont2 = zs_asset_find_by_sid(asset_finder, s2);
    zs_contract_t* pcont3 = zs_asset_find_by_sid(asset_finder, s3);
    assert(pcont1);
    assert(pcont2);
    assert(pcont3);

    rv = zs_asset_del(asset_finder, ZS_EI_SHFE, "rb1905", 6);
    assert(rv == 0);
    pcont1 = zs_asset_find(asset_finder, ZS_EI_SHFE, "rb1905", 6);
    assert(!pcont1);
    pcont2 = zs_asset_find(asset_finder, ZS_EI_SSE, "000001", 6);
    assert(pcont2);
    pcont2 = zs_asset_find(asset_finder, ZS_EI_SZSE, "000001", 6);
    assert(pcont2);

    // test multiple
    int n = 0;
    int exchangeid = 0;
    const int count = 10000;
    zs_sid_t sid_arr[10000] = { 0 };
    for (int i = 0; i < count; ++i)
    {
        char buf[32] = "";
        int len = 0;
        if (i < count / 2) {
            exchangeid = ZS_EI_SZSE;
            len = sprintf(buf, "%06d", i + 1);
        }
        else {
            exchangeid = ZS_EI_SSE;
            // len = sprintf(buf, "6%05d", i);
            len = sprintf(buf, "%06d", i);
        }

        zs_sid_t sid = 0;
        zs_contract_t contract;
        memset(&contract, 0, sizeof(contract1));
        strncpy(contract.Symbol, buf, len);
        contract.ExchangeID = exchangeid;
        rv = zs_asset_add_copy(asset_finder, &sid, contract.ExchangeID, contract.Symbol, len, &contract, sizeof(contract));

        zs_contract_t* pcont = zs_asset_find_by_sid(asset_finder, sid);
        pcont->Sid = sid;
        pcont->ExchangeID = exchangeid;
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
        if (i < count / 2) {
            exchangeid = ZS_EI_SZSE;
            len = sprintf(buf, "%06d", i + 1);
        }
        else {
            exchangeid = ZS_EI_SSE;
            // len = sprintf(buf, "6%05d", i);
            len = sprintf(buf, "%06d", i);
        }
        assert(strcmp(buf, pcont->Symbol) == 0);
        assert(pcont->ExchangeID == exchangeid);
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
    date_str_to_tm(&ltm1, date1, (int)strlen(date1));
    date_str_to_tm(&ltm2, date2, (int)strlen(date2));
    datetime_str_to_tm(&ltm3, datetime1, (int)strlen(datetime1));
    datetime_str_to_tm(&ltm4, datetime2, (int)strlen(datetime2));

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

void test_broker_info()
{
    setlocale(LC_CTYPE, "");

    const char* filename = "brokers.json";
    ztl_vector_t vec;
    ztl_vector_init(&vec, 2, sizeof(zs_broker_info_t));
    zs_broker_info_load(&vec, filename);

    zs_broker_info_t* broker_info;
    broker_info = zs_broker_find_byid(&vec, "0118");
    broker_info = zs_broker_find_byname(&vec, "九洲期货");
    broker_info = NULL;
}

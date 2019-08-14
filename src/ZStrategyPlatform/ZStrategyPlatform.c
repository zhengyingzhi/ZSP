#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ZToolLib/ztl_common.h>
#include <ZToolLib/ztl_config.h>

#include <ZStrategyAPI/zs_algorithm.h>
#include <ZStrategyAPI/zs_common.h>
#include <ZStrategyAPI/zs_core.h>
#include <ZStrategyAPI/zs_configs.h>
#include <ZStrategyAPI/zs_constants_helper.h>
#include <ZStrategyAPI/zs_data_portal.h>

#include <ZStrategyAPI/zs_strategy_demo.h>
#include <ZStrategyAPI/zs_strategy_engine.h>

#include <ZStrategyAPI/zs_logger.h>

static int32_t conv_time_str(const char* stime)
{
    int32_t t;
    t = atoi(stime) * 10000 + atoi(stime + 3) * 100 + atoi(stime + 6);
    return t;
}

static int _parse_line_tick_data(zs_csv_loader_t* csv_loader, int num, zditem_t fields[], int size)
{
    zs_tick_t*      tick;
    ztl_pool_t*     pool;
    ztl_array_t*    array;
    uint32_t        alloc_size;

    if (num == 0) {
        return ZS_OK;
    }

    pool = (ztl_pool_t*)csv_loader->userdata1;
    array = (ztl_array_t*)csv_loader->userdata2;

    alloc_size = ztl_align(sizeof(zs_tick_t), 8);
    tick = (zs_tick_t*)ztl_pcalloc(pool, alloc_size);
    // tick = (zs_tick_t*)calloc(1, alloc_size);

    // retrieve each field
    tick->ExchangeID = zs_convert_exchange_name(fields[1].ptr, fields[1].len);
    strncpy(tick->Symbol, fields[2].ptr, fields[2].len);
    tick->TradingDay = (int32_t)atoi_n(fields[3].ptr, fields[3].len);
    tick->ActionDay = (int32_t)atoi_n(fields[4].ptr, fields[4].len);
    tick->UpdateTime = conv_time_str(fields[5].ptr) * 1000 + (int32_t)atoi_n(fields[5].ptr + 9, 3);

    tick->OpenPrice = atof(fields[6].ptr);
    tick->HighPrice = atof(fields[7].ptr);
    tick->LowPrice = atof(fields[8].ptr);
    tick->LastPrice = atof(fields[9].ptr);
    tick->Volume = atoi(fields[10].ptr);
    tick->Turnover = atof(fields[11].ptr);
    tick->OpenInterest = atof(fields[13].ptr);
    tick->ClosePrice = atof(fields[14].ptr);
    tick->UpperLimit = atof(fields[15].ptr);
    tick->LowerLimit = atof(fields[16].ptr);
    tick->PreClosePrice = atof(fields[17].ptr);
    tick->PreSettlementPrice = atof(fields[18].ptr);
    tick->PreOpenInterest = atof(fields[19].ptr);
    tick->SettlementPrice = atof(fields[20].ptr);
    tick->BidPrice[0] = atof(fields[22].ptr);
    tick->BidVolume[0] = atoi(fields[23].ptr);
    tick->AskPrice[0] = atof(fields[24].ptr);
    tick->AskVolume[0] = atoi(fields[25].ptr);

    ztl_array_push_back(array, &tick);
    return ZS_OK;
}

int main(int argc, char* argv[])
{
    ZTL_NOTUSED(argc);
    ZTL_NOTUSED(argv);

    printf("zsp version:%s\n", ZS_Version);

    /* 启动交易/回测 */

    /* 
       先加载配置文件，加载历史数据（回测或实盘交易所需的历史数据）
       回测：
       a. 加载基准数据
       b. 根据起止日期，回测频率来创建交易日历

       交易(包括回测，回测时API类型需要指定为)
       1. 创建回测对象Algorithm
       2. 指定所需要使用的API（分交易和行情，且允许使用多个API）
       3. 传入条件参数
       4. 根据配置，加载策略并初始化
       4. 开始运行（启动策略，启动API，事件引擎）
       5. 运行结束（释放相关资源），得到交易报告并展示（且要持久化到文件，供python分析）
     */

    int rv;

    ztl_pool_t* pool;
    pool = ztl_create_pool(ZTL_DEFAULT_POOL_SIZE);

    // read config
    zs_algo_param_t params;
    memset(&params, 0, sizeof(params));
    zs_algo_param_init(&params);
    rv = zs_configs_load(&params, pool);
    // assert(rv == 0);

    zs_data_portal_t* data_portal;
    data_portal = zs_data_portal_create();

#if 1
    // load history ohlc, benchmark data
    ztl_array_t ohlc_datas;
    ztl_array_init(&ohlc_datas, NULL, 48000, sizeof(zs_tick_t*));

    zs_csv_loader_t csv_loader;
    memset(&csv_loader, 0, sizeof(csv_loader));
    csv_loader.filename = "rb1910_md_20190731.csv";
    csv_loader.parse_line_fields = _parse_line_tick_data;
    csv_loader.have_header = 1;
    csv_loader.is_tick_data = 1;
    csv_loader.sep = ",";
    csv_loader.userdata1 = pool;
    csv_loader.userdata2 = &ohlc_datas;
    rv = zs_data_load_csv(&csv_loader);
    assert(rv == ZS_OK);

    //ztl_array_t benchmark_datas;
    //zs_data_load_csv("000300SH.csv", &benchmark_datas);
    //assert(rv == 0);

    // data_portal = zs_data_portal_create();
    // zs_data_portal_wrapper(data_portal, &ohlc_datas);
    // zs_data_portal_wrapper(data_portal, &benchmark_datas);

    data_portal->RawDatas = &ohlc_datas;
    data_portal->IsTick = 1;
    params.RunMode = ZS_RM_Backtest;
#endif

    // run algo
    zs_algorithm_t* algo;
    algo = zs_algorithm_create(&params);

    // we can mannually add some settings, like broker, account, strategy setting etc.

    // test demo strategy
    zs_strategy_entry_t* strategy_entry = NULL;
    zs_demo_strategy_entry(&strategy_entry);
    zs_algorithm_add_strategy_entry(algo, strategy_entry);

    // broker info
    zs_conf_broker_t conf_broker = { 0 };
    strncpy(conf_broker.APIName, "CTP", sizeof(conf_broker.APIName));
    strncpy(conf_broker.BrokerID, "9999", sizeof(conf_broker.BrokerID));
    strncpy(conf_broker.BrokerName, "SimNow", sizeof(conf_broker.BrokerName));
    strncpy(conf_broker.TradeAddr, "tcp://180.168.146.187:10101", sizeof(conf_broker.TradeAddr));
    strncpy(conf_broker.MDAddr, "tcp://180.168.146.187:10111", sizeof(conf_broker.MDAddr));
    zs_algorithm_add_broker_info2(algo, &conf_broker);

    // account info
    zs_conf_account_t account_conf = { 0 };
    strncpy(account_conf.AccountID, "038313", sizeof(account_conf.AccountID));
    strncpy(account_conf.AccountName, "yizhe", sizeof(account_conf.AccountName));
    strncpy(account_conf.Password, "qwert", sizeof(account_conf.Password));
    strncpy(account_conf.BrokerID, "9999", sizeof(account_conf.BrokerID));
    strncpy(account_conf.TradeAPIName, "CTP", sizeof(account_conf.TradeAPIName));
    strncpy(account_conf.MDAPIName, "CTP", sizeof(account_conf.MDAPIName));
    account_conf.AuthCode = "";
    // strcpy(account_conf.AppID, "", sizeof(account_conf.AppID));
    // strcpy(account_conf.AuthCode, "", sizeof(account_conf.AuthCode));
    zs_algorithm_add_account2(algo, &account_conf);

    // !example code! add one strategy (by json buffer)
    const char* strategy_setting;
    if (params.RunMode == ZS_RM_Backtest) {
        strategy_setting = "{\"Symbol\": \"rb1910\", \"StrategyName\": \"strategy_demo\", \"AccountID\": \"backtest\"}";
    }
    else {
        strategy_setting = "{\"Symbol\": \"rb1910\", \"StrategyName\": \"strategy_demo\", \"AccountID\": \"038313\"}";
    }
    zs_algorithm_add_strategy(algo, strategy_setting);

    // init the algorithm
    rv = zs_algorithm_init(algo);
    assert(rv == 0);

    // start running
    rv = zs_algorithm_run(algo, data_portal);

    char ch;
    while (1)
    {
        ch = getchar();
        if (ch == 'e' || ch == 'q') {
            break;
        }

        // TODO: input some cmds to operate zs algorithm

        if (ch == 's')
        {
            zs_strategy_start_all(algo->StrategyEngine, NULL);
            continue;
        }

        if (ch < '0' || ch > '5') {
            continue;
        }

        zs_cta_strategy_t* strategy;
        strategy = zs_strategy_find_byid(algo->StrategyEngine, 1);
        if (!strategy) {
            continue;
        }

        int flag = ch - '0';
        char buffer[1000] = "";
        sprintf(buffer, "{\"flag\":%d}", flag);

        zs_strategy_update(algo->StrategyEngine, strategy, buffer);
    }
    fprintf(stderr, "zs algorithm stopping.\n");
    zs_algorithm_stop(algo);
    fprintf(stderr, "zs algorithm stopped.\n");

    // get run result from algo
    ztl_array_t result;
    ztl_array_init(&result, NULL, 1024, sizeof(void*));
    zs_algorithm_result(algo, &result);

    /* 
      // 回测资金, 账户信息，策略名字，均由配置文件获取
      // data_frequency, product_type(应该由具体代码具体), start_date, end_date
      // 运行模式（跨进程等）（创建各模块）

     */

    return 0;
}




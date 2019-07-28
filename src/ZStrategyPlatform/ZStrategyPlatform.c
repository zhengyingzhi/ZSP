#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ZToolLib/ztl_common.h>
#include <ZToolLib/ztl_config.h>

#include <ZStrategyAPI/zs_algorithm.h>
#include <ZStrategyAPI/zs_common.h>
#include <ZStrategyAPI/zs_core.h>
#include <ZStrategyAPI/zs_configs.h>
#include <ZStrategyAPI/zs_data_portal.h>

#include <ZStrategyAPI/zs_strategy_demo.h>

//#include <ZDataRepository>


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
    rv = zs_configs_load(&params, pool);
    // assert(rv == 0);

    zs_data_portal_t* data_portal;
    data_portal = NULL;

#if 0
    // load ohlc,benchmark data
    ztl_array_t ohlc_datas;
    ztl_array_init(&ohlc_datas, pool, 32, sizeof(zs_bar_t*));
    zs_data_load_csv("IF000.csv", &ohlc_datas);
    assert(rv == 0);

    //ztl_array_t benchmark_datas;
    //zs_data_load_csv("000300SH.csv", &benchmark_datas);
    //assert(rv == 0);

    data_portal = zs_data_portal_create();
    zs_data_portal_wrapper(data_portal, &ohlc_datas);
    //zs_data_portal_wrapper(data_portal, &benchmark_datas);
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
    strncpy(conf_broker.TradeAddr, "tcp://180.168.146.187:10100", sizeof(conf_broker.TradeAddr));
    strncpy(conf_broker.MDAddr, "tcp://180.168.146.187:10110", sizeof(conf_broker.MDAddr));
    zs_algorithm_add_broker_info2(algo, &conf_broker);

    // account info
    zs_conf_account_t account_conf = { 0 };
    strncpy(account_conf.AccountID, "038313", sizeof(account_conf.AccountID));
    strncpy(account_conf.AccountName, "yizhe", sizeof(account_conf.AccountName));
    strncpy(account_conf.Password, "qwert", sizeof(account_conf.Password));
    strncpy(account_conf.BrokerID, "9999", sizeof(account_conf.BrokerID));
    strncpy(account_conf.TradeAPIName, "CTP", sizeof(account_conf.TradeAPIName));
    strncpy(account_conf.MDAPIName, "CTP", sizeof(account_conf.MDAPIName));
    // strcpy(account_conf.AppID, "", sizeof(account_conf.AppID));
    // strcpy(account_conf.AuthCode, "", sizeof(account_conf.AuthCode));
    zs_algorithm_add_account2(algo, &account_conf);

    // !example code! add one strategy (by json buffer)
    const char* strategy_setting = "{\"Symbol\": \"rb1910\", \"StrategyName\": \"strategy_demo\", \"account_id\": \"00100002\"}";
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
    }
    fprintf(stderr, "zs algorithm stopping.");
    zs_algorithm_stop(algo);
    fprintf(stderr, "zs algorithm stopped.");

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




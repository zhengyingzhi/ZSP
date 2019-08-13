#include <assert.h>

#include "zs_algorithm.h"
#include "zs_backtest.h"


int zs_backtest_run(zs_backtest_t* backtest)
{
    /*
     1. 准备回测相关环境
     2. 创建algorithm对象
     3. 开始回测
     4. 得到回测结果
     */

    int rv;

    ztl_pool_t* pool;
    pool = ztl_create_pool(ZTL_DEFAULT_POOL_SIZE);

    // read config
    zs_algo_param_t params;
    memset(&params, 0, sizeof(params));
    params.RunMode = ZS_RM_Backtest;
    zs_algo_param_init(&params);

    // backtest broker api
    zs_conf_broker_t* broker_conf;
    broker_conf = (zs_conf_broker_t*)ztl_pcalloc(pool, sizeof(zs_conf_broker_t));
    strcpy(broker_conf->BrokerID, "0000");
    strcpy(broker_conf->BrokerName, "INNER");
    strcpy(broker_conf->APIName, "backtest");
    ztl_array_push_back(&params.BrokerConf, &broker_conf);

    zs_conf_account_t account_conf = { 0 };
    strncpy(account_conf.AccountID, "00000000", sizeof(account_conf.AccountID));
    strncpy(account_conf.AccountName, "backtester", sizeof(account_conf.AccountName));
    strncpy(account_conf.Password, "", sizeof(account_conf.Password));
    strncpy(account_conf.BrokerID, "0000", sizeof(account_conf.BrokerID));
    strncpy(account_conf.TradeAPIName, "backtest", sizeof(account_conf.TradeAPIName));
    strncpy(account_conf.MDAPIName, "backtest", sizeof(account_conf.MDAPIName));
    ztl_array_push_back(&params.AccountConf, &account_conf);

    // the algo object
    zs_algorithm_t* algo;
    algo = zs_algorithm_create(&params);

    // the backtest account
    zs_algorithm_add_account2(algo, &account_conf);

    // the backtest strategy entry
    zs_algorithm_add_strategy_entry(algo, backtest->StrategyEntry);

    // the backtest strategy setting
    zs_algorithm_add_strategy(algo, backtest->StrategySetting);

    // init the algorithm
    rv = zs_algorithm_init(algo);
    assert(rv == 0);

    // start running
    rv = zs_algorithm_run(algo, NULL /* data_portal */);

    return rv;
}

void zs_backtest_release(zs_backtest_t* backtest)
{
    if (backtest->Algorithm)
    {
        zs_algorithm_release(backtest->Algorithm);
        backtest->Algorithm = NULL;

        ztl_destroy_pool(backtest->Pool);
    }
}

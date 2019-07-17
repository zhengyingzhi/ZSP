#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <ZToolLib/ztl_common.h>
#include <ZToolLib/ztl_config.h>

#include <ZStrategyAPI/zs_algorithm.h>

#include <ZStrategyAPI/zs_common.h>
#include <ZStrategyAPI/zs_core.h>

#include <ZStrategyAPI/zs_configs.h>

#include "ZStrategyAPI/zs_data_portal.h"

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
    const char* conf_file = "zs_config.json";
    zs_algo_param_t params;
    memset(&params, 0, sizeof(params));
    rv = zs_configs_load(&params, pool);
    assert(rv == 0);

    // load ohlc,benchmark data
    ztl_array_t ohlc_datas;
    ztl_array_init(&ohlc_datas, pool, 32, sizeof(zs_bar_t*));
    zs_data_load_csv("IF000.csv", &ohlc_datas);
    assert(rv == 0);

    //ztl_array_t benchmark_datas;
    //zs_data_load_csv("000300SH.csv", &benchmark_datas);
    //assert(rv == 0);

    zs_data_portal_t* data_portal;
    data_portal = zs_data_portal_create();
    zs_data_portal_wrapper(data_portal, &ohlc_datas);
    //zs_data_portal_wrapper(data_portal, &benchmark_datas);

    // run algo
    zs_algorithm_t* algo;
    algo = zs_algorithm_create(&params);

    rv = zs_algorithm_init(algo);
    assert(rv == 0);

    rv = zs_algorithm_run(algo, data_portal);

    // get run result from algo
    ztl_array_t result;
    ztl_array_init(&result, algo->Pool, 1024, sizeof(void*));
    zs_algorithm_result(algo, &result);


    /* 
      // 回测资金, 账户信息，策略名字，均由配置文件获取
      // data_frequency, product_type(应该由具体代码具体), start_date, end_date
      // 运行模式（跨进程等）（创建各模块）

     */

    return 0;
}




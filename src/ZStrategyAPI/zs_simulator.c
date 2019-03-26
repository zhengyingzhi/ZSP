
#include <ZToolLib/ztl_threads.h>
#include <ZToolLib/ztl_utils.h>

#include "zs_algorithm.h"

#include "zs_algorithm_api.h"

#include "zs_broker_backtest.h"

#include "zs_simulator.h"

#include "zs_slippage.h"


// deprecated
typedef enum
{
    ZS_SEV_PreRun,
    ZS_SEV_SesstionStart,
    ZS_SEV_Bar,
    ZS_SEV_SessionEnd,
    ZS_SEV_PostRun
}ZSSimuEvent;


// 处理slippage的回调，并回调通知到api_handler(即broker_entry里定义的)
static void _zs_slippage_handler(zs_slippage_t* slippage, ZSSlippageDataType dataType,
    void* data, int dataSize);
// 处理模拟器的新行情，并回调通知到api_handler(即broker_entry里定义的)
static void _zs_simu_newmd_handler(zs_simulator_t* simu, zs_bar_reader_t* barData);


zs_simulator_t* zs_simulator_create(zs_algorithm_t* algo)
{
    zs_simulator_t* simu;

    simu = (zs_simulator_t*)ztl_pcalloc(algo->Pool, sizeof(zs_simulator_t));

    simu->Algorithm     = algo;
    simu->DataPortal    = algo->DataPortal;
    simu->Slippage      = zs_slippage_create(ZS_PFF_Close, _zs_slippage_handler, simu);

    simu->TdApi = NULL;
    simu->MdApi = NULL;

    simu->Progress = 0;
    simu->Running = 0;

    return simu;
}

void zs_simulator_release(zs_simulator_t* simu)
{
    if (simu)
    {
        if (simu->Slippage) {
            zs_slippage_release(simu->Slippage);
            simu->Slippage = NULL;
        }
    }
}

void zs_simulator_regist_tradeapi(zs_simulator_t* simu,
    zs_trade_api_t* tdapi, zs_trade_api_handlers_t* tdHandlers)
{
    simu->TdApi = tdapi;
    simu->TdHandlers = tdHandlers;
}

void zs_simulator_regist_mdapi(zs_simulator_t* simu,
    zs_md_api_t* mdapi, zs_md_api_handlers_t* mdHandlers)
{
    simu->MdApi = mdapi;
    simu->MdHandlers = mdHandlers;
}

void zs_simulator_stop(zs_simulator_t* simu)
{
    simu->Running = 0;
}

int zs_simulator_run(zs_simulator_t* simu)
{
    simu->Running = 1;

    int64_t currentDt = 20180800000000;
    zs_bar_reader_t bar_reader;
    zs_bar_reader_init(&bar_reader, simu->Algorithm->DataPortal);

    // 开始前，通知上层回测事件开始
    //if (simu->Handler)
    //    simu->Handler(simu, ZS_SEV_PreRun);

    while (simu->Running)
    {
        /* 根据起止日期，从交易日历的时间信息，及回测时间段，然后生成各个事件ZSSimuEvent
         */

        ZSSimuEvent sev = ZS_SEV_Bar;

        currentDt += 1000000;

        // 更新时间
        bar_reader.CurrentDt = currentDt;

        if (sev == ZS_SEV_Bar)
        {
            // 获取到新bar行情，先撮合
            zs_slippage_process_bybar(simu->Slippage, &bar_reader);

            // 再通过broker md handler通知到交易核心
            _zs_simu_newmd_handler(simu, &bar_reader);
        }

        sleepms(1000);
    }

    // 结束时
    //if (simu->Handler)
    //    simu->Handler(simu, ZS_SEV_PostRun);

    return 0;
}



static void _zs_slippage_handler(zs_slippage_t* slippage, 
    ZSSlippageDataType dataType, void* data, int dataSize)
{
    zs_simulator_t* simu;
    zs_trade_api_handlers_t* td_handlers;

    simu = (zs_simulator_t*)slippage->UserData;
    td_handlers = simu->TdHandlers;

    if (dataType == ZS_SDT_Order)
    {
        zs_order_t* order = (zs_order_t*)data;

        if (td_handlers->on_rtn_order)
            td_handlers->on_rtn_order(simu->TdApi, order);
    }
    else if (dataType == ZS_SDT_Trade)
    {
        zs_trade_t* trade = (zs_trade_t*)data;

        if (td_handlers->on_rtn_trade)
            td_handlers->on_rtn_trade(simu->TdApi, trade);
    }
}

static void _zs_simu_newmd_handler(zs_simulator_t* simu, zs_bar_reader_t* barReader)
{
    zs_md_api_handlers_t* md_handlers;
    md_handlers = simu->MdHandlers;

#if 0
    if (md_handlers->on_bardata)
        md_handlers->on_bardata(simu->MdApi, barReader, 0);
#endif
}



zs_trade_api_t* zs_sim_backtest_trade_api()
{
    return &bt_tdapi;
}

zs_md_api_t* zs_sim_backtest_md_api()
{
    return &bt_mdapi;
}

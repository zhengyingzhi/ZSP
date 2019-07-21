#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ZStrategyAPI/zs_algorithm.h>
#include <ZStrategyAPI/zs_assets.h>
#include <ZStrategyAPI/zs_cta_strategy.h>
#include <ZStrategyAPI/zs_data_portal.h>
#include <ZStrategyAPI/zs_strategy_entry.h>

#include <ZStrategyAPI/zs_position.h>
#include <ZStrategyAPI/zs_protocol.h>



#if defined(_MSC_VER) && defined(DOUBLEMA_ISLIB)
#ifdef DOUBLEMA_EXPORTS
#define DMA_API __declspec(dllexport)
#else
#define DMA_API __declspec(dllimport)
#endif
#define DMA_API_STDCALL __stdcall   /* ensure stcall calling convention on NT */

#else
#define DMA_API
#define DMA_API_STDCALL             /* leave blank for other systems */

#endif//_WIN32

typedef struct strategy_dma_s
{
    int             index;
    zs_sid_t        sid;
    ZSExchangeID    exchangeid;
    char            accountid[16];

    // conf params
    char            symbol[32];
    char            period[8];
    int32_t         period_int;
    int32_t         fast_window;
    int32_t         slow_window;
    int32_t         volume;
    int32_t         slippage;
    int32_t         print_tick;
    int32_t         is_daily;
    double          max_symbol_pos;
    double          max_symol_loss_ratio;
    double          max_loss_pervol;

    // strategy variables
    double          fast_ma0;
    double          fast_ma1;
    double          slow_ma0;
    double          slow_ma1;
}strategy_dma_t;

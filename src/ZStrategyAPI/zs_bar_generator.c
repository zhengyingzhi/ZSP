
#include "zs_bar_generator.h"


zs_bar_generator_t* zs_bargen_create(ztl_pool_t* pool, uint64_t sid)
{
    zs_bar_generator_t* bargen;
    bargen = (zs_bar_generator_t*)ztl_palloc(pool, sizeof(zs_bar_generator_t));
    zs_bargen_init(bargen, sid);
    return bargen;
}

void zs_bargen_init(zs_bar_generator_t* bargen, uint64_t sid)
{
    memset(bargen, 0, sizeof(zs_bar_generator_t));
    bargen->IsFinished  = 1;
    bargen->IsFinishedX = 1;
    bargen->Sid         = sid;
}

void zs_bargen_release(zs_bar_generator_t* bargen)
{
}


void zs_bargen_set_handle_bar(zs_bar_generator_t* bargen, void* userdata,
    handle_bar_pt handle_bar_func,
    handle_bar_pt handle_barx_func)
{
    bargen->UserData = userdata;
    bargen->handle_bar = handle_bar_func;
    bargen->handle_barx = handle_barx_func;
}

void zs_bargen_update_tick(zs_bar_generator_t* bargen, zs_tick_t* tick)
{
    int generated = 0;
    zs_bar_t* bar = &bargen->CurrentBar;

    if (bargen->IsFinished)
    {
        bargen->IsFinished = 0;
        strcpy(bar->Period, "1m");
        strcpy(bar->Symbol, tick->Symbol);
        bar->ExchangeID = tick->ExchangeID;

        bar->OpenPrice = tick->LastPrice;
        bar->ClosePrice = tick->LastPrice;
        bar->HighPrice = tick->LastPrice;
        bar->LowPrice = tick->LowPrice;
        bar->Volume = 0;
    }
    else if (bar->BarDt.minute != tick->TickDt.minute)
    {
        // generated a new bar
        generated = 1;

        // this bar is finished
        bargen->IsFinished = 1;
    }

    if (1)
    {
        if (bar->HighPrice < tick->LastPrice)
            bar->HighPrice = tick->LastPrice;
        if (bar->LowPrice > tick->LowPrice)
            bar->LowPrice = tick->LastPrice;
        bar->ClosePrice = tick->LastPrice;
        bar->Volume += tick->Volume - bargen->LastTick.Volume;
        bar->BarDt = tick->TickDt;

        if (tick->TickDt.minute != bargen->LastTick.TickDt.minute)
        {
            bar->BarDt.second = 0;
            bar->BarDt.millisec = 0;
            bargen->IsFinished = 1;

            if (bargen->handle_bar)
                bargen->handle_bar(bargen->UserData, bar);
        }
    }

    // only update some fields of tick
    bargen->LastTick.Volume = tick->Volume;
}

void zs_bargen_update_tickl2(zs_bar_generator_t* bargen, zs_tickl2_t* tickl2)
{
}

void zs_bargen_update_bar(zs_bar_generator_t* bargen, zs_bar_t* bar)
{
    zs_bar_t* barx = &bargen->CurrentBarX;
    if (bargen->IsFinishedX)
    {
        bargen->IsFinishedX = 0;
        barx->OpenPrice = bar->OpenPrice;
        barx->HighPrice = bar->HighPrice;
        barx->LowPrice = bar->LowPrice;
        barx->ClosePrice = bar->ClosePrice;
        barx->Volume = 0;
        barx->BarDt = bar->BarDt;
        sprintf(barx->Period, "%dm", bargen->XMin);
    }
    else
    {
        if (barx->HighPrice < bar->HighPrice)
            barx->HighPrice = bar->HighPrice;
        if (barx->LowPrice > bar->LowPrice)
            barx->LowPrice = bar->LowPrice;
        barx->Volume += bar->Volume;
        bar->Volume = 0;
        bar->BarDt = bar->BarDt;

        if (bar->BarDt.minute % bargen->XMin == 0)
        {
            bargen->IsFinishedX = 1;
            barx->BarDt = bar->BarDt;

            if (bargen->handle_barx) {
                bargen->handle_barx(bargen->UserData, barx);
            }
        }
    }
}



#include "zs_bar_generator.h"


zs_bar_generator_t* zs_bar_gen_create(ztl_pool_t* pool, uint64_t sid)
{
    zs_bar_generator_t* bargen;
    bargen = (zs_bar_generator_t*)ztl_palloc(pool, sizeof(zs_bar_generator_t));
    zs_bar_gen_init(bargen, sid);
    return bargen;
}

void zs_bar_gen_init(zs_bar_generator_t* bargen, uint64_t sid)
{
    memset(bargen, 0, sizeof(bargen));
    bargen->IsFinished = 0;
    bargen->Sid = sid;
}

void zs_bar_gen_release(zs_bar_generator_t* bargen)
{
}

void zs_bar_gen_update_tick(zs_bar_generator_t* bargen, zs_tick_t* tick)
{
    zs_bar_t* bar = &bargen->CurrentBar;
    if (bargen->IsFinished)
    {
        bargen->IsFinished = 0;
        bar->OpenPrice = tick->LastPrice;
        bar->ClosePrice = tick->LastPrice;
        bar->HighPrice = tick->LastPrice;
        bar->LowPrice = tick->LowPrice;
        bar->Volume += tick->Volume - bargen->LastTick.Volume;
    }
    else
    {
        if (tick->LastPrice > bar->HighPrice)
            bar->HighPrice = tick->LastPrice;
        else if (tick->LowPrice < bar->LowPrice)
            bar->LowPrice = tick->LastPrice;
        bar->ClosePrice = tick->LastPrice;
        bar->Volume = 0;
        bar->BarDt = tick->TickDt;

        if (tick->TickDt.minute != bargen->LastTick.TickDt.minute)
        {
            bar->BarDt.second = 0;
            bar->BarDt.millisec = 0;
            bargen->IsFinished = 1;

            if (bargen->handle_bar_pt)
                bargen->handle_bar_pt(bargen->UserData, bar);
        }
    }

    // only update some fields of tick
    bargen->LastTick.Volume = tick->Volume;
}

void zs_bar_gen_update_tickl2(zs_bar_generator_t* bargen, zs_tickl2_t* tickl2)
{
}

void zs_bar_gen_update_bar(zs_bar_generator_t* bargen, zs_bar_t* bar)
{
    zs_bar_t* barx = &bargen->CurrentBarX;
    if (bargen->IsFinished)
    {
        bargen->IsFinished = 0;
        barx->OpenPrice = bar->OpenPrice;
        barx->ClosePrice = bar->ClosePrice;
        barx->HighPrice = bar->HighPrice;
        barx->LowPrice = bar->LowPrice;
        barx->Volume += bar->Volume;
    }
    else
    {
        if (bar->HighPrice > barx->HighPrice)
            barx->HighPrice = bar->HighPrice;
        else if (bar->LowPrice < barx->LowPrice)
            barx->LowPrice = bar->LowPrice;
        bar->Volume = 0;
        bar->BarDt = bar->BarDt;

        if (bar->BarDt.minute % bargen->XMin == 0)
        {
            bargen->IsFinishedX = 1;
            barx->BarDt = bar->BarDt;

            if (bargen->handle_barx_pt)
                bargen->handle_barx_pt(bargen->UserData, barx);
        }
    }
}


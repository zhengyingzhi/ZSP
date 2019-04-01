
#include "zs_bar_generator.h"


zs_bar_generator_t* zs_bar_gen_create(ztl_pool_t* pool)
{
    zs_bar_generator_t* bargen;
    bargen = (zs_bar_generator_t*)ztl_pcalloc(pool, sizeof(zs_bar_generator_t));
    return bargen;
}

void zs_bar_gen_init(zs_bar_generator_t* bargen)
{
}

void zs_bar_gen_release(zs_bar_generator_t* bargen)
{
}

void zs_bar_gen_update_tick(zs_bar_generator_t* bargen, zs_tick_t* tick)
{
}

void zs_bar_gen_update_tickl2(zs_bar_generator_t* bargen, zs_tickl2_t* tickl2)
{
}

void zs_bar_gen_update_bar(zs_bar_generator_t* bargen, zs_bar_t* bar)
{
}



#include <ZToolLib/ztl_atomic.h>

#ifdef _MSC_VER
#include <Windows.h>
#else
#endif

#include "zs_common.h"

#include "zs_core.h"


uint32_t zs_data_increment(zs_data_head_t* zdh)
{
    return ztl_atomic_add(&zdh->RefCount, 1);
}

uint32_t zs_data_decre_release(zs_data_head_t* zdh)
{
    uint32_t oldv = ztl_atomic_dec(&zdh->RefCount, 1);
    if (1 == oldv)
    {
        if (zdh->Cleanup)
            zdh->Cleanup(zdh);
    }
    return oldv;
}

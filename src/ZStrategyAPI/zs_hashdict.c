#include <ZToolLib/ztl_utils.h>
#include <ZToolLib/ztl_memcpy.h>

#include "zs_hashdict.h"


dictType uintHashDictType = {
    zs_uint_hash,
    NULL,
    NULL,
    zs_uint_cmp,
    NULL,
    NULL
};

dictType strHashDictType = {
    zs_str_hash,
    NULL,       // zs_str_keydup,
    NULL,
    zs_str_cmp,
    NULL,
    NULL
};


void* zs_str_keydup(const ZStrKey* key, void*(alloc_pt)(void*, size_t), void* alloc_ctx)
{
    ZStrKey* skey = (ZStrKey*)key;
    ZStrKey* dup_key = (ZStrKey*)alloc_pt(alloc_ctx, ztl_align(sizeof(ZStrKey) + skey->len, 4));
    dup_key->len = skey->len;
    dup_key->ptr = (char*)(dup_key + 1);
    ztl_memcpy(dup_key->ptr, skey->ptr, skey->len);
    return dup_key;
}

void* zs_str_keydup2(const char* str, int length, void*(alloc_pt)(void*, size_t), void* alloc_ctx)
{
    ZStrKey* dup_key = (ZStrKey*)alloc_pt(alloc_ctx, ztl_align(sizeof(ZStrKey) + length + 1, sizeof(void*)));
    dup_key->len = length;
    dup_key->ptr = (char*)(dup_key + 1);
    dup_key->ptr[length] = '\0';
    ztl_memcpy(dup_key->ptr, str, length);
    return dup_key;
}

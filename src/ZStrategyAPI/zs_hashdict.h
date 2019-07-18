/*
 * Copyright (C) Yingzhi Zheng.
 * Copyright (C) <zhengyingzhi112@163.com>
 * define the common hash dict type
 */

#ifndef _ZS_HASH_DICT_H_
#define _ZS_HASH_DICT_H_

#include <stdint.h>
#include <string.h>

#include <ZToolLib/ztl_dict.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef dict ztl_dict_t;


typedef struct
{
    int len;
    char* ptr;
}ZStrKey;


static uint64_t zs_uint_hash(const void *key) {
    return (uint64_t)key;
}
static int zs_uint_cmp(void* priv, const void* s1, const void* s2) {
    uint64_t u1 = (uint64_t)s1;
    uint64_t u2 = (uint64_t)s2;
    if (u1 == u2) {
        return 1;
    }
    return 0;
}

/* a simple hash dict type based on uint64
 */
extern dictType uintHashDictType;

/* a simple hash dict type based on string without key dup
 */
extern dictType strHashDictType;


static uint64_t zs_str_hash(const void *key) {
    ZStrKey* skey = (ZStrKey*)key;
    return dictGenHashFunction((unsigned char*)skey->ptr, skey->len);
}

static int zs_str_cmp(void* priv, const void* s1, const void* s2) {
    (void)priv;
    ZStrKey* k1 = (ZStrKey*)s1;
    ZStrKey* k2 = (ZStrKey*)s2;
    return (k1->len == k2->len) && memcmp((ZStrKey*)k1->ptr, k2->ptr, k2->len) == 0;
}

void* zs_str_keydup(const ZStrKey* key, void*(alloc_pt)(void*, size_t), void* alloc_ctx);


#ifdef __cplusplus
}
#endif

#endif//_ZS_HASH_DICT_H_

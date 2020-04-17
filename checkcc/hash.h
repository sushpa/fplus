/* The MIT License

   Copyright (c) 2008, 2009, 2011 by Attractive Chaos <attractor@live.co.uk>

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

#ifndef HAVE_DICT
#define HAVE_DICT

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define empty(flag, i) ((flag[i >> 4] >> ((i & 0xfU) << 1)) & 2)
#define deleted(flag, i) ((flag[i >> 4] >> ((i & 0xfU) << 1)) & 1)
#define emptyOrDel(flag, i) ((flag[i >> 4] >> ((i & 0xfU) << 1)) & 3)

#define setNotDeleted(flag, i) (flag[i >> 4] &= ~(1ul << ((i & 0xfU) << 1)))
#define setDeleted(flag, i) (flag[i >> 4] |= 1ul << ((i & 0xfU) << 1))

#define setNotEmpty(flag, i) (flag[i >> 4] &= ~(2ul << ((i & 0xfU) << 1)))
#define clearFlags(flag, i) (flag[i >> 4] &= ~(3ul << ((i & 0xfU) << 1)))

#define flagsSize(m) ((m) < 16 ? 1 : (m) >> 4)

#define roundUp32(x)                                                       \
    (--(x), (x) |= (x) >> 1, (x) |= (x) >> 2, (x) |= (x) >> 4,             \
        (x) |= (x) >> 8, (x) |= (x) >> 16, ++(x))

static const double HASH_UPPER = 0.77;

// Sets do not have any values, but the type of the value ptr is char*.

// Generally it is not recommended to use Dict_has because you call it and
// then you call Dict_get again. So just call Dict_get and do whatever.
// If you don't really want the index, then its fine to call Dict_has.
#define Dict(K, V) Dict_##K##_##V
#define Dict_init(K, V) Dict_init_##K##_##V
#define Dict_free(K, V) Dict_free_##K##_##V
#define Dict_freedata(K, V) Dict_freedata_##K##_##V
#define Dict_clear(K, V) Dict_clear_##K##_##V
#define Dict_resize(K, V) Dict_resize_##K##_##V
#define Dict_put(K, V) Dict_put_##K##_##V
#define Dict_get(K, V) Dict_get_##K##_##V
#define Dict_has(K, V) Dict_has_##K##_##V
#define Dict_del(K, V) Dict_del_##K##_##V

#define Set(K) Dict(K, char)
#define Set_init(K) Dict_init(K, char)
#define Set_free(K) Dict_free(K, char)
#define Set_freedata(K) Dict_freedata(K, char)
#define Set_clear(K) Dict_clear(K, char)
#define Set_resize(K) Dict_resize(K, char)
#define Set_put(K) Dict_put(K, char)
#define Set_get(K) Dict_get(K, char)
#define Set_has(K) Dict_has(K, char)
#define Set_del(K) Dict_del(K, char)

// TODO: do it without any templating at all, void* keys, values, etc.

#define __DICT_TYPE(K, V)                                                  \
    typedef struct Dict(K, V)                                              \
    {                                                                      \
        UInt32 nBuckets, size, nOccupied, upperBound;                      \
        UInt32* flags;                                                     \
        K* keys;                                                           \
        V* vals;                                                           \
    }                                                                      \
    Dict(K, V);

// #define __DICT_PROTOTYPES(K, V)                                            \
//     Dict(K, V) * Dict_init(K, V)();                                        \
//     void Dict_free(K, V)(Dict(K, V) * h);                                  \
//     void Dict_freedata(K, V)(Dict(K, V) * h);                              \
//     void Dict_clear(K, V)(Dict(K, V) * h);                                 \
//     UInt32 Dict_get(K, V)(const Dict(K, V) * const h, K key);                    \
//     bool Dict_has(K, V)(const Dict(K, V) * const h, K key);                      \
//     int Dict_resize(K, V)(Dict(K, V) * h, UInt32 nnBuckets);               \
//     UInt32 Dict_put(K, V)(Dict(K, V) * h, K key, int* ret);                \
//     void Dict_del(K, V)(Dict(K, V) * h, UInt32 x);

#define __DICT_IMPL(Scope, K, V, IsMap, hash, equal)                       \
    Scope Dict(K, V) * Dict_init(K, V)()                                   \
    {                                                                      \
        return calloc(1, sizeof(Dict(K, V)));                              \
    }                                                                      \
    Scope void Dict_freedata(K, V)(Dict(K, V) * h)                         \
    {                                                                      \
        if (h) {                                                           \
            free(h->keys);                                                 \
            free(h->flags);                                                \
            if (IsMap) free(h->vals);                                      \
        }                                                                  \
    }                                                                      \
    Scope void Dict_free(K, V)(Dict(K, V) * h) { free(h); }                \
    Scope void Dict_clear(K, V)(Dict(K, V) * h)                            \
    {                                                                      \
        if (h and h->flags) {                                              \
            memset(                                                        \
                h->flags, 0xAA, flagsSize(h->nBuckets) * sizeof(UInt32));  \
            h->size = h->nOccupied = 0;                                    \
        }                                                                  \
    }                                                                      \
    Scope UInt32 Dict_get(K, V)(const Dict(K, V)* const h, K key)          \
    {                                                                      \
        if (h->nBuckets) {                                                 \
            UInt32 k, i, last, mask, step = 0;                             \
            mask = h->nBuckets - 1;                                        \
            k = hash(key);                                                 \
            i = k & mask;                                                  \
            last = i;                                                      \
            while (not empty(h->flags, i)                                  \
                and (deleted(h->flags, i)                                  \
                    or not equal(h->keys[i], key))) {                      \
                i = (i + (++step)) & mask;                                 \
                if (i == last) return h->nBuckets;                         \
            }                                                              \
            return emptyOrDel(h->flags, i) ? h->nBuckets : i;              \
        } else                                                             \
            return 0;                                                      \
    }                                                                      \
    Scope bool Dict_has(K, V)(const Dict(K, V)* const h, K key)            \
    {                                                                      \
        UInt32 x = Dict_get(K, V)(h, key);                                 \
        return x < h->nBuckets && Dict_exist(h, x);                        \
    }                                                                      \
    Scope int Dict_resize(K, V)(Dict(K, V) * h, UInt32 nnBuckets)          \
    { /* This function uses 0.25*nBuckets bytes of working space instead   \
         of [sizeof(key_t+val_t)+.25]*nBuckets. */                         \
        UInt32* nFlags = 0;                                                \
        UInt32 j = 1;                                                      \
        {                                                                  \
            roundUp32(nnBuckets);                                          \
            if (nnBuckets < 4) nnBuckets = 4;                              \
            if (h->size >= (UInt32)(nnBuckets * HASH_UPPER + 0.5))         \
                j = 0; /* requested size is too small */                   \
            else { /* size to be changed (shrink or expand); rehash */     \
                nFlags = malloc(flagsSize(nnBuckets) * sizeof(UInt32));    \
                if (not nFlags) return -1;                                 \
                memset(                                                    \
                    nFlags, 0xAA, flagsSize(nnBuckets) * sizeof(UInt32));  \
                if (h->nBuckets < nnBuckets) { /* expand */                \
                    K* nKeys = realloc(h->keys, nnBuckets * sizeof(K));    \
                    if (not nKeys) {                                       \
                        free(nFlags);                                      \
                        return -1;                                         \
                    }                                                      \
                    h->keys = nKeys;                                       \
                    if (IsMap) {                                           \
                        V* nVals                                           \
                            = realloc(h->vals, nnBuckets * sizeof(V));     \
                        if (not nVals) {                                   \
                            free(nFlags);                                  \
                            return -1;                                     \
                        }                                                  \
                        h->vals = nVals;                                   \
                    }                                                      \
                } /* otherwise shrink */                                   \
            }                                                              \
        }                                                                  \
        if (j) { /* rehashing is needed */                                 \
            for (j = 0; j != h->nBuckets; ++j) {                           \
                if (emptyOrDel(h->flags, j) == 0) {                        \
                    K key = h->keys[j];                                    \
                    V val;                                                 \
                    UInt32 new_mask;                                       \
                    new_mask = nnBuckets - 1;                              \
                    if (IsMap) val = h->vals[j];                           \
                    setDeleted(h->flags, j);                               \
                    while (1) { /* kick-out process; sort of like in       \
                                   Cuckoo hashing */                       \
                        UInt32 k, i, step = 0;                             \
                        k = hash(key);                                     \
                        i = k & new_mask;                                  \
                        while (not empty(nFlags, i))                       \
                            i = (i + (++step)) & new_mask;                 \
                        setNotEmpty(nFlags, i);                            \
                        if (i < h->nBuckets                                \
                            and emptyOrDel(h->flags, i) == 0) {            \
                            /* kick out the existing element */            \
                            {                                              \
                                K tmp = h->keys[i];                        \
                                h->keys[i] = key;                          \
                                key = tmp;                                 \
                            }                                              \
                            if (IsMap) {                                   \
                                V tmp = h->vals[i];                        \
                                h->vals[i] = val;                          \
                                val = tmp;                                 \
                            }                                              \
                            setDeleted(h->flags, i);                       \
                            /* mark it deleted in the old table */         \
                        } else {                                           \
                            /* write the element and break the loop */     \
                            h->keys[i] = key;                              \
                            if (IsMap) h->vals[i] = val;                   \
                            break;                                         \
                        }                                                  \
                    }                                                      \
                }                                                          \
            }                                                              \
            if (h->nBuckets > nnBuckets) { /* shrink the hash table */     \
                h->keys = realloc(h->keys, nnBuckets * sizeof(K));         \
                if (IsMap)                                                 \
                    h->vals = realloc(h->vals, nnBuckets * sizeof(V));     \
            }                                                              \
            free(h->flags); /* free the working space */                   \
            h->flags = nFlags;                                             \
            h->nBuckets = nnBuckets;                                       \
            h->nOccupied = h->size;                                        \
            h->upperBound = (UInt32)(h->nBuckets * HASH_UPPER + 0.5);      \
        }                                                                  \
        return 0;                                                          \
    }                                                                      \
    Scope UInt32 Dict_put(K, V)(Dict(K, V) * h, K key, int* ret)           \
    {                                                                      \
        UInt32 x;                                                          \
        if (h->nOccupied >= h->upperBound) { /* update the hash table */   \
            if (h->nBuckets > (h->size << 1)) {                            \
                if (Dict_resize(K, V)(h, h->nBuckets - 1) < 0) {           \
                    /* clear "deleted" elements */                         \
                    *ret = -1;                                             \
                    return h->nBuckets;                                    \
                }                                                          \
            } else if (Dict_resize(K, V)(h, h->nBuckets + 1) < 0) {        \
                /* expand the hash table */                                \
                *ret = -1;                                                 \
                return h->nBuckets;                                        \
            }                                                              \
        } /* TODO: to implement automatically shrinking; resize() already  \
             support shrinking */                                          \
        {                                                                  \
            UInt32 k, i, site, last, mask = h->nBuckets - 1, step = 0;     \
            x = site = h->nBuckets;                                        \
            k = hash(key);                                                 \
            i = k & mask;                                                  \
            if (empty(h->flags, i))                                        \
                x = i; /* for speed up */                                  \
            else {                                                         \
                last = i;                                                  \
                while (not empty(h->flags, i)                              \
                    and (deleted(h->flags, i)                              \
                        or not equal(h->keys[i], key))) {                  \
                    if (deleted(h->flags, i)) site = i;                    \
                    i = (i + (++step)) & mask;                             \
                    if (i == last) {                                       \
                        x = site;                                          \
                        break;                                             \
                    }                                                      \
                }                                                          \
                if (x == h->nBuckets) {                                    \
                    if (empty(h->flags, i) and site != h->nBuckets)        \
                        x = site;                                          \
                    else                                                   \
                        x = i;                                             \
                }                                                          \
            }                                                              \
        }                                                                  \
        if (empty(h->flags, x)) { /* not present at all */                 \
            h->keys[x] = key;                                              \
            clearFlags(h->flags, x);                                       \
            ++h->size;                                                     \
            ++h->nOccupied;                                                \
            *ret = 1;                                                      \
        } else if (deleted(h->flags, x)) { /* deleted */                   \
            h->keys[x] = key;                                              \
            clearFlags(h->flags, x);                                       \
            ++h->size;                                                     \
            *ret = 2;                                                      \
        } else                                                             \
            *ret = 0;                                                      \
        /* Don't touch h->keys[x] if present and not deleted */            \
        return x;                                                          \
    }                                                                      \
    Scope void Dict_del(K, V)(Dict(K, V) * h, UInt32 x)                    \
    {                                                                      \
        if (x != h->nBuckets and not emptyOrDel(h->flags, x)) {            \
            setDeleted(h->flags, x);                                       \
            --h->size;                                                     \
        }                                                                  \
    }

#define DICT_DECLARE(K, V)                                                 \
    __DICT_TYPE(K, V)                                                      \
    __DICT_PROTOTYPES(K, V)

#define DICT_INIT2(Scope, K, V, IsMap, hash, equal)                        \
    __DICT_TYPE(K, V)                                                      \
    __DICT_IMPL(Scope, K, V, IsMap, hash, equal)

#define DICT_INIT(K, V, IsMap, hash, equal)                                \
    DICT_INIT2(static inline, K, V, IsMap, hash, equal)

/* --- BEGIN OF HASH FUNCTIONS --- */

#define UInt32_hash(key) (UInt32)(key)

#define UInt32_equal(a, b) ((a) == (b))

#define UInt64_hash(key) (UInt32)((key) >> 33 ^ (key) ^ (key) << 11)

#define UInt64_equal(a, b) ((a) == (b))

#define Real64_hash(key) UInt64_hash((UInt64)key)

#define Real64_equal(a, b) ((a) == (b))

static inline UInt32 __ac_X31_hash_string(const char* s)
{
    UInt32 i = (UInt32)*s;
    if (i)
        for (++s; *s; ++s)
            i = (i << 5) - i + (UInt32)*s;
    return i;
}

#define CString_hash(key) __ac_X31_hash_string(key)

#define CString_equal(a, b) (strcmp(a, b) == 0)
// for CStrings, assuming they MUST end in \0, you can check for
// pointer equality to mean string equality. For Strings with lengths,
// this does not hold since the same buffer can hold a string and any of
// its prefixes.

static inline UInt32 __ac_Wang_hash(UInt32 key)
{
    key += ~(key << 15);
    key ^= (key >> 10);
    key += (key << 3);
    key ^= (key >> 6);
    key += ~(key << 11);
    key ^= (key >> 16);
    return key;
}
#define Dict_int_hash_func2(key) __ac_Wang_hash((UInt32)key)

#define Dict_exist(h, x) (not emptyOrDel((h)->flags, (x)))

#define Dict_key(h, x) ((h)->keys[x])

#define Dict_val(h, x) ((h)->vals[x])

#define Dict_begin(h) (UInt32)(0)

#define Dict_end(h) ((h)->nBuckets)

#define Dict_size(h) ((h)->size)

#define Dict_nBuckets(h) ((h)->nBuckets)

#define Dict_foreach(h, kvar, vvar, code)                                  \
    {                                                                      \
        for (UInt32 i = Dict_begin(h); i != Dict_end(h); ++i) {            \
            if (not Dict_exist(h, i)) continue;                            \
            kvar = Dict_key(h, i);                                         \
            vvar = Dict_val(h, i);                                         \
            code;                                                          \
        }                                                                  \
    }

#define Dict_foreach_value(h, vvar, code)                                  \
    {                                                                      \
        for (UInt32 i = Dict_begin(h); i != Dict_end(h); ++i) {            \
            if (not Dict_exist(h, i)) continue;                            \
            vvar = Dict_val(h, i);                                         \
            code;                                                          \
        }                                                                  \
    }

#define Set_foreach(h, kvar, code) Dict_foreach_key(h, kvar, code)
#define Dict_foreach_key(h, kvar, code)                                    \
    {                                                                      \
        for (UInt32 i = Dict_begin(h); i != Dict_end(h); ++i) {            \
            if (not Dict_exist(h, i)) continue;                            \
            kvar = Dict_key(h, i);                                         \
            code;                                                          \
        }                                                                  \
    }

#define MAKE_SET(T) DICT_INIT(T, char, false, T##_hash, T##_equal)
#define MAKE_DICT(K, V) DICT_INIT(K, V, true, K##_hash, K##_equal)

MAKE_SET(UInt32)
// MAKE_SET(Real64)
MAKE_SET(CString)
// MAKE_SET(Ptr)

MAKE_DICT(UInt32, UInt32)
MAKE_DICT(CString, UInt32)
// MAKE_DICT(UInt32, Real64)
// MAKE_DICT(CString, Real64)
// MAKE_DICT(UInt32, Ptr)
// MAKE_DICT(CString, Ptr)
MAKE_DICT(UInt32, CString)
MAKE_DICT(CString, CString)

// #define MAKE_SET_INT                                                       \
//     DICT_INIT(UInt32, char, false, Dict_UInt32_hash, Dict_UInt32_equal)

// #define MAKE_MAP_INT(V)                                                    \
//     DICT_INIT(UInt32, V, true, Dict_UInt32_hash, Dict_UInt32_equal)

// #define MAKE_SET_INT64                                                     \
//     DICT_INIT(UInt64, char, false, Dict_UInt64_hash, Dict_UInt64_equal)

// #define MAKE_MAP_INT64(V)                                                  \
//     DICT_INIT(UInt64, V, true, Dict_UInt64_hash, Dict_UInt64_equal)

// typedef const char* CString;

// #define MAKE_SET_STR                                                       \
//     DICT_INIT(CString, char, false, Dict_CString_hash, Dict_CString_equal)

// #define MAKE_MAP_STR(V)                                                    \
//     DICT_INIT(CString, V, true, Dict_CString_hash, Dict_CString_equal)

#endif /* HAVE_DICT */
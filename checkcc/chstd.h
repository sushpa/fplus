
#ifndef chstd_h
#define chstd_h

#define and &&
#define or ||
#define not !

#define min(a, b) ((a) < (b)) ? (a) : (b)
#define max(a, b) ((a) > (b)) ? (a) : (b)
#define KB *1024UL

#define eprintf(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#define eputs(str) fputs(str, stderr)

#pragma mark - Heap allocation stuff

static int globalCallocCount = 0;
#define calloc(n, s)                                                       \
    calloc(n, s);                                                          \
    globalCallocCount++;
static int globalMallocCount = 0;
#define malloc(s)                                                          \
    malloc(s);                                                             \
    globalMallocCount++;
static int globalStrdupCount = 0;
#define strdup(s)                                                          \
    strdup(s);                                                             \
    globalStrdupCount++;
static int globalReallocCount = 0;
#define realloc(ptr, s)                                                    \
    realloc(ptr, s);                                                       \
    globalReallocCount++;
static int globalStrlenCount = 0;
#define strlen(s)                                                          \
    strlen(s);                                                             \
    globalStrlenCount++;

#pragma mark - Custom types
typedef char bool;
#define true 1
#define false 0
typedef int Int;
typedef double Scalar;
typedef char** CStrings;

#pragma mark - Variant

union Value {
    // think about returning larger things like Interval etc.
    char* s;
    int64_t i;
    uint64_t u;
    double d;
};

#pragma mark - Array

//#define DEFAULT0(T) DEFAULT0_##T
//#define DEFAULT0_double 0.0
//#define DEFAULT0_float 0.0
//#define DEFAULT0_uint64_t 0
//#define DEFAULT0_uint32_t 0
//#define DEFAULT0_int32_t 0
//#define DEFAULT0_int64_t 0
//#define DEFAULT0_voidptr NULL

typedef void* Ptr;
typedef uint32_t UInt32;
typedef uint64_t UInt64;
typedef const char* CString;
typedef double Real64;

#define Array(T) Array_##T
#define Array_free(T) Array_free_##T
#define Array_get(T) Array_get_##T
#define Array_growTo(T) Array_growTo_##T
#define Array_concat_cArray(T) Array_concat_cArray_##T
#define Array_concat_otherArray(T) Array_concat_otherArray_##T
#define Array_grow(T) Array_grow_##T
#define Array_push(T) Array_push_##T
#define Array_justPush(T) Array_justPush_##T
#define Array_clear(T) Array_clear_##T
#define Array_initWith_cArray(T) Array_initWith_cArray_##T
#define Array_pop(T) Array_pop_##T
#define Array_top(T) Array_top_##T
#define Array_empty(T) Array_empty_##T

// convenience for manual writing
#define PtrArray Array(Ptr)
// #define PtrArray_free Array_free(Ptr)
// #define PtrArray_growTo Array_growTo(Ptr)
// #define PtrArray_concat_cArray Array_concat_cArray(Ptr)
// #define PtrArray_concat_otherArray Array_concat_otherArray(Ptr)
// #define PtrArray_grow Array_grow(Ptr)
#define PtrArray_push Array_push(Ptr)
// #define PtrArray_clear Array_clear(Ptr)
// #define PtrArray_initWithCArray Array_initWithCArray(Ptr)
// #define PtrArray_justPush Array_justPush(Ptr)
#define PtrArray_pop Array_pop(Ptr)
#define PtrArray_top Array_top(Ptr)
#define PtrArray_empty Array_empty(Ptr)
#define PtrArray_topAs(T, this) ((T)PtrArray_top(this))

// #define PtrArray_topAs(T, this) Array_topAs(T, this)

#define roundUp32(x)                                                       \
    (--(x), (x) |= (x) >> 1, (x) |= (x) >> 2, (x) |= (x) >> 4,             \
        (x) |= (x) >> 8, (x) |= (x) >> 16, ++(x))

// dont get smart and try to do Array(Array(Array(whatever)))

// Should be using adhoc to generate these.

// At some point, make these use some smart ass tricks to enable good
// perf even with -O0 since -O takes too long -- doesn't seem like "instant"
// typically 200-300 ms should be the upper limit for compilation time
// YES "COMPILATION" MEANS TO BINARY NOT TO C, that should be < 20ms
// for a typical project.
#define MAKE_Array(T)                                                      \
    typedef struct Array(T)                                                \
    {                                                                      \
        T* ref;                                                            \
        UInt32 used;                                                       \
        UInt32 cap;                                                        \
    }                                                                      \
    Array(T);                                                              \
    void Array_free(T)(Array(T) * this)                                    \
    {                                                                      \
        if (this->cap) free(this->ref);                                    \
    }                                                                      \
    void Array_growTo(T)(Array(T) * this, UInt32 size)                     \
    {                                                                      \
        this->cap = roundUp32(size);                                       \
        this->ref = realloc(this->ref, sizeof(T) * this->cap);             \
        memset(this->ref + this->used, 0,                                  \
            sizeof(T) * (this->cap - this->used));                         \
    }                                                                      \
    T Array_get(T)(Array(T) * this, UInt32 index)                          \
    {                                                                      \
        return this->ref[index];                                           \
    }                                                                      \
    void Array_concat_cArray(T)(Array(T) * this, T * cArray, int count)    \
    {                                                                      \
        const UInt32 reqd = this->used + count;                            \
        if (reqd >= this->cap) Array_growTo(T)(this, reqd);                \
        memcpy(this->ref + this->used, cArray, count * sizeof(T));         \
    }                                                                      \
    void Array_concat_otherArray(T)(Array(T) * this, Array(T) * other)     \
    {                                                                      \
        Array_concat_cArray(T)(this, other->ref, other->used);             \
    }                                                                      \
    void Array_clear(T)(Array(T) * this) { this->used = 0; }               \
    void Array_initWith_cArray(T)(Array(T) * this, T * cArray, int count)  \
    {                                                                      \
        Array_clear(T)(this);                                              \
        Array_concat_cArray(T)(this, cArray, count);                       \
    }                                                                      \
    void Array_grow(T)(Array(T) * this)                                    \
    { /* maybe this can be merged with growTo */                           \
        this->cap = this->cap ? 2 * this->cap : 8;                         \
        this->ref = realloc(this->ref, sizeof(T) * this->cap);             \
        memset(this->ref + this->used, 0,                                  \
            sizeof(T) * (this->cap - this->used));                         \
    }                                                                      \
    void Array_justPush(T)(Array(T) * this, T node)                        \
    { /* when you know that cap is enough */                               \
        this->ref[this->used++] = node;                                    \
    }                                                                      \
    void Array_push(T)(Array(T) * this, T node)                            \
    {                                                                      \
        if (this->used >= this->cap) Array_grow(T)(this);                  \
        Array_justPush(T)(this, node);                                     \
    }                                                                      \
    T Array_pop(T)(Array(T) * this)                                        \
    {                                                                      \
        assert(this->used > 0);                                            \
        return this->ref[--this->used];                                    \
    }                                                                      \
    T Array_top(T)(Array(T) * this)                                        \
    {                                                                      \
        return this->used ? this->ref[this->used - 1] : 0;                 \
    }                                                                      \
    bool Array_empty(T)(Array(T) * this) { return this->used == 0; }

MAKE_Array(Ptr);
MAKE_Array(UInt32);
// MAKE_Array(uint64_t);
// MAKE_Array(int64_t);
// MAKE_Array(int32_t);
MAKE_Array(Scalar);
// MAKE_Array(float);
// make array for strings etc later

// Array_top(T) is only defined for value types to keep the number
// of instantiations (of the "template" Array) down. So void* represents
// object ptrs of all types. Cast them when you need to deref or do -> etc.
// this is used to get a void* as a T (usually a SomeType*)

// TODO: StaticArray type with size and array, StaticArray2D/3Detc.
// since this is not templated, it's your job to send items of the
// right size, or face the music
// ASSUMING SIZE IS 8. THAT MEANS NO FLOAT OR UINT32, only sizeof(void*)
// #define Array_concat_cArray(T, Array, arr, count) \
//     Array_concat_cArray_(Array, arr, count * sizeof(T))
// TODO: the compiler should optimise away calls to concat if the original
// arrays can be used one after the other. e.g. concat two arrays then print
// it can be done by simply printing first then second, no memcpy involved.
// #define Array_concat_otherArray(T, s1, s2) \
//     Array_concat_otherArray_(s1, s2, sizeof(T))
#pragma mark - Pool

typedef struct PoolB {
    void* ref;
    UInt32 cap, capTotal; // BYTES
    Array(Ptr) ptrs;
    UInt32 used, usedTotal; // used BYTES, unlike in Array!
} PoolB;

void* PoolB_alloc(PoolB* this, size_t reqd)
{
    void* ans = NULL;

    // This is a pool for single objects, not arrays or large strings.
    // dont ask for a big fat chunk larger than 16KB (or up to 256KB
    // depending on how much is already there) all at one time.
    if (this->used + reqd > this->cap) {
        if (this->ref) Array_push(Ptr)(&this->ptrs, this->ref);
        this->cap = (this->cap > 64 KB ? 256 KB : 4 KB);
        this->capTotal += this->cap;
        this->ref = calloc(1, this->cap);
        assert(this->ref != NULL);
        this->used = 0;
    }
    ans = (void*)((uintptr_t)this->ref + this->used);
    this->used += reqd;
    this->usedTotal += reqd;
    return ans;
}

void PoolB_free(PoolB* this)
{
    // TODO: reset used here?
    if (this->cap) free(this->ref);
    for (int i = 0; i < this->ptrs.used; i++)
        free(this->ptrs.ref[i]);
}

PoolB gPool;
PoolB strPool;

#define NEW(T)                                                             \
    PoolB_alloc(&gPool, sizeof(T));                                        \
    T##_allocTotal++;

// This macro should be invoked on each struct defined.
#define MKSTAT(T) static int T##_allocTotal = 0;

#pragma mark - PtrList

MKSTAT(PtrList)

#define List(T) PtrList
typedef struct PtrList {
    void** item;
    struct PtrList* next;
} PtrList;

PtrList* PtrList_with(void* item)
{
    // TODO: how to get separate alloc counts of List_ASTType List_ASTFunc
    // etc.?
    PtrList* li = NEW(PtrList);
    li->item = item;
    return li;
}

int PtrList_count(PtrList* listPtr)
{
    int i = 0;
    while (listPtr) {
        listPtr = listPtr->next;
        i++;
    }
    return i;
}

void PtrList_append(PtrList** selfp, void* item)
{
    if (*selfp == NULL) // first append call
        *selfp = PtrList_with(item);
    else {
        PtrList* this = *selfp;
        while (this->next)
            this = this->next;
        this->next = PtrList_with(item);
    }
}

#define foreach(T, var, listp, listSrc)                                    \
    PtrList* listp = listSrc;                                              \
    if (listp)                                                             \
        for (T var = (T)listp->item; listp && (var = (T)listp->item);      \
             listp = listp->next)

#endif /* chstd_h */

#pragma mark - String Functions

#include "strcasecmp.h"

#define str_endswith(str, lenstr, suffix, lensuffix)                       \
    !strncmp(str + lenstr - lensuffix, suffix, lensuffix)

#define str_startswith(str, prefix, lenprefix)                             \
    !strncmp(str, prefix, lenprefix)

// DO NOT USE strdup,strndup,strcasecmp,strncasecmp: OK reimplemented
// strcasecmp.

char* pstrndup(char* str, size_t len)
{
    char* ret = PoolB_alloc(&strPool, len + 1);
    memcpy(ret, str, len); // strPool uses calloc, so no need to zero last
    return ret;
}

char* pstrdup(char* str)
{
    const size_t len = strlen(str);
    return pstrndup(str, len);
}

char* str_noext(char* str)
{
    const size_t len = strlen(str);
    char* s = pstrndup(str, len);
    char* sc = s + len;
    while (sc > s and *sc != '.')
        sc--;
    if (sc >= s) *sc = '\0';
    return s;
}

char* str_base(char* str, char sep, size_t slen)
{
    if (!slen)
        return str; // you should pass in the len. len 0 is actually valid
                    // since basename for 'mod' is 'mod' itself, and this
                    // would have caused a call to strlen below. so len 0
                    // now means really just return what came in.
    char* s = str;
    char* sc = s + slen;
    while (sc > s and sc[-1] != sep)
        sc--;
    if (sc >= s) s = sc;
    return s;
}

char* str_dir(char* str)
{
    const size_t len = strlen(str);
    char* s = pstrndup(str, len);
    char* sc = s + len;
    while (sc > s and *sc != '/')
        sc--;
    if (sc >= s) *sc = '\0';
    return s;
}

char* str_upper(char* str)
{
    char* s = pstrdup(str);
    char* sc = s - 1;
    while (*++sc)
        if (*sc >= 'a' and *sc <= 'z') *sc -= 32;
    return s;
}

// in place
void str_tr_ip(
    char* str, const char oldc, const char newc, const size_t length)
{
    char* sc = str - 1;
    char* end = length ? str + length : (char*)0xFFFFFFFFFFFFFFFF;
    while (*++sc && sc < end)
        if (*sc == oldc) *sc = newc;
}

char* str_tr(char* str, const char oldc, const char newc)
{
    size_t len = strlen(str);
    char* s = pstrndup(str, len);
    str_tr_ip(s, oldc, newc, len);
    return s;
}

char* str_nthField(char* str, int len, char sep, int nth) { return NULL; }

int str_countFields(char* str, int len, char sep) { return 0; }

// caller sends target as stack array or NULL
char** str_getAllOccurences(char* str, int len, char sep, int* count)
{
    // result will be malloced & realloced
    return 0;
}

int str_getSomeOccurences(
    char* str, int len, char sep, char** result, int limit)
{
    // result buf is expected from caller
    return 0;
}

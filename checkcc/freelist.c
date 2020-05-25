#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include "fp_base.h"
#include "hash.h"

// tODO: this is a heap checker that reports whether what is allocated is
// freed. also need a mem profiler that will report what the mem usage is at
// any given location as a % of the total mem AT THE POINT IN TIME just
// after it is allocated.

static int EL = 0;
static const char* spc = "                ";
#ifdef TRACE
#define EP                                                                     \
    printf("%.*s%s -> %s:%d\n", 2 * EL++, spc, __func__, __FILE__, __LINE__);
#define EX EL--;
#else
#define EP
#define EX
#endif
MKSTAT(FLSizeClassInfo)

typedef struct FLSizeClassInfo {
    PtrList* list;
    int count;
} FLSizeClassInfo;
// size -> FLSizeClassInfo* map
static Dict(UInt64, Ptr) szDict[1];

#define STR(X) #X
#define SPOT(x, fn, fil, lin) fil ":" STR(lin) ": " x

#define fp_dealloc(x)                                                          \
    fp_dealloc_(x, sizeof(*x), SPOT(#x, __func__, __FILE__, __LINE__))
#define fp_heap_free(x) fp_heap_free_(x, SPOT(#x, __func__, __FILE__, __LINE__))
static void fp_dealloc_(void* ptr, UInt64 size, const char* desc)
// just put into freelist
{
    EP;
    int stat[1];
    int d = Dict_put(UInt64, Ptr)(szDict, size, stat);
    if (not Dict_val(szDict, d)) Dict_val(szDict, d) = NEW(FLSizeClassInfo);
    FLSizeClassInfo* inf = Dict_val(szDict, d);
    // if (inf->count>=32) { /* really free it if on heap using fp_heap_free */}
    // else
    // {
    inf->list = PtrList_with_next(ptr, inf->list);
    inf->count++;
    // }
    EX
}

// TODO: fp_alloc should not be affected: disabling challoc merely makes
// challoc, fp_heap_free identical to malloc,free.
// TODO: need 2 switches, one to turn off heap checker and one to just make
// everything fall back to malloc/free (no pool).

#ifdef FP_NOHEAPCHECK // --------------------------------------------------
#define fp_alloc(nam, sz) fp_alloc_(sz, "")
void* fp_heap_malloc(UInt64 size, const char* desc) { return malloc(size); }
#define fp_heap_free free
void* fp_alloc_(UInt64 size, const char* desc)
{
    return fp_heap_malloc(size, "");
}
UInt64 fp_heap_report() { return 0; }

#else // FP_NOHEAPCHECK ---------------------------------------------------//
      // TODO: the compiler will call fp_alloc_ directly with a custom built
      // string.
#define fp_alloc(nam, sz) fp_alloc_(sz, SPOT(nam, __func__, __FILE__, __LINE__))
static UInt64 fp_heap_total = 0;
typedef struct FLPtrInfo {
    UInt64 size : 63, heap : 1;
    const char* desc;
} FLPtrInfo;
MAKE_DICT(Ptr, FLPtrInfo)
// void* -> FLPtrInfo map
static Dict(Ptr, FLPtrInfo) ptrDict[1];

typedef struct {
    UInt64 count, sum;
} countsum;
MAKE_DICT(CString, countsum)

Dict(CString, countsum) locwise[1] = {};

int fp_heap_cmpfn(const void* a, const void* b)
{
    // For sorting in descending order of sum
    UInt64 va = Dict_val(locwise, *(int*)a).sum;
    UInt64 vb = Dict_val(locwise, *(int*)b).sum;
    return va > vb ? -1 : va == vb ? 0 : 1;
}

// void* fp_heap_malloc(UInt64 size, const char* desc)
// {
//     EP;
//     void* ret = malloc(size);
//     // if (ret) {
//     //     int stat[1];
//     //     FLPtrInfo inf = { //
//     //         .size = size,
//     //         .heap = true,
//     //         .desc = desc
//     //     };
//     //     UInt32 p = Dict_put(Ptr, FLPtrInfo)(ptrDict, ret, stat);
//     //     Dict_val(ptrDict, p) = inf;
//     //     fp_heap_total += size;
//     // }
//     EX;
//     return ret;
// }

void fp_heap_free_(void* ptr, const char* desc)
{
    EP;
    // int stat[1];
    int d = Dict_get(Ptr, FLPtrInfo)(ptrDict, ptr);
    // use put so we can still access deleted items
    // printf("%d %d\n", d, ptrDict->nBuckets);
    // if (stat == 2) {

    if (d < Dict_end(ptrDict)) {
        // else {
        if (not Dict_val(ptrDict, d).heap) {
            printf("free non-heap pointer at %s\n", desc);
        } else {
            free(ptr);
            fp_heap_total -= Dict_val(ptrDict, d).size;
            Dict_del(Ptr, FLPtrInfo)(ptrDict, d);
        }
    } else {
        printf("double free attempted at %s\n", desc);
    }
    EX
}

// TODO: this func is not so great for strings, as it is "exact-fit". Freed
// pointers are reused only for objects of exactly the same size as the
// pointer's previous data. This is nearly useless for strings. They have their
// own string pool, you should probably have a fp_alloc_str that is best-fit.
void* fp_alloc_(UInt64 size, const char* desc)
// alloc from freelist, or pool or heap, in that order of preference
{
    EP;
#ifndef NOMEMPOOL
    int d = Dict_get(UInt64, Ptr)(szDict, size);
    if (d < Dict_end(szDict)) {
        FLSizeClassInfo* inf = Dict_val(szDict, d);
        if (inf->list) {
            void* ret = inf->list->item;
            fp_dealloc(inf->list);
            inf->list = inf->list->next;
            inf->count--;
            return ret;
        }
    }
    // see if it can fit in the pool, if yes, allocate from there.
    // if size is smallish <= N, force an allocation from the pool.
    // If current subpool doesn't have enough bytes, it'll calloc
    // a new subpool, you'll lose at most N-1 bytes.
    bool fromPool = size <= 256 or size <= (gPool->cap - gPool->used);
    void* ret = fromPool //
        ? PoolB_alloc(gPool, size)
        // else fall back to the heap allocator.
        : malloc(size); // fp_heap_malloc(size, desc);
    if (not fromPool) fp_heap_total += size;
#else
    void* ret = malloc(size);
#ifndef NOHEAPTOTAL
    fp_heap_total += size;
#endif
#endif

#ifndef NOHEAPTRACKER
    if (ret) {
        int stat[1];
        FLPtrInfo inf = { //
            .size = size,
            .heap = not fromPool,
            .desc = desc
        };
        UInt32 p = Dict_put(Ptr, FLPtrInfo)(ptrDict, ret, stat);
        Dict_val(ptrDict, p) = inf;
    }
#endif

    EX;
    return ret;
}

UInt64 fp_heap_report()
{
    EP;
    int i = 0;
    UInt64 sum = 0;
    if (Dict_size(ptrDict)) {
        int stat[1];

        Dict_foreach(ptrDict, Ptr key, FLPtrInfo val, {
            if (not val.heap) continue;
            int d = Dict_put(CString, countsum)(locwise, val.desc, stat);
            if (*stat) {
                Dict_val(locwise, d).sum = 0;
                Dict_val(locwise, d).count = 0;
            }
            Dict_val(locwise, d).sum += val.size;
            Dict_val(locwise, d).count++;
            sum += val.size;
        });

        if (locwise->size) {
            int i = 0, *indxs = malloc(locwise->size * sizeof(int));
            Dict_foreach(
                locwise, CString key, countsum val, { indxs[i++] = _i_; });
            qsort(indxs, locwise->size, sizeof(int), fp_heap_cmpfn);
            puts("-------------------------------------------------------------"
                 "-");
            printf("Allocations | Total Size (B) | Variable and Location\n");
            puts("-------------------------------------------------------------"
                 "-");
            for (i = 0; i < locwise->size; i++) {
                CString key = locwise->keys[indxs[i]];
                countsum val = locwise->vals[indxs[i]];
                printf("%11llu | %14llu | %s\n", val.count, val.sum, key);
            }
            UInt64 rss = gPool->capTotal;
            // if (sum) {
            puts("---------------------------------------------------------"
                 "-----");
            printf("Heap residual %llu bytes (%.2f%% of pool total %llu "
                   "bytes)\n",
                sum, sum * 100.0 / rss, rss);
            // }
            Dict_clear(CString, countsum)(locwise);
        } else {
            puts("-- no leaks.");
        }
    }

    EX;
    return sum;
}

#endif // FP_NOHEAPCHECK --------------------------------------------------

int main()
{
    EP;
    double* d3 = fp_alloc("d3[] as Real", 64 * sizeof(double));
    double* d = fp_alloc("d[] as ~kg.m/s2", 8 * sizeof(double));
    double* d2 = fp_alloc("d2 as MyType", 32 * sizeof(double));
    double *e, *ex;
    for (int i = 0; i < 132; i++) {
        e = fp_alloc("e as String", 384 * sizeof(char));
        //...
        // if (i > 13)
        // fp_dealloc(e);
        fp_heap_free(e);
        // ex = fp_alloc("ex as String", 14 * i * sizeof(char));
        // }
    }

    // fp_heap_free(d3);
    fp_heap_free(d3);
    fp_heap_free(d);
    fp_heap_free(d2);
    // typeof(*d) dc = d[0];
    // printf("%s", typeof(*d));
    // WHY REPORT? JUST WALK THE LIST AND FREE THEM BEFORE EXIT.
    // REPORT IS ONLY USEFUL TO SEE WHAT COULD HAVE BEEN RELEASED WHILE
    // THE PROGRAM WAS RUNNING, TO GIVE BACK MEM to the system.
    // SO DON'T CALL IT "LEAKED", JUST "LEFT OVER"
    UInt64 s = fp_heap_report();

    EX
}
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
MKSTAT(fp_mem__SizeClassInfo)

typedef struct fp_mem__SizeClassInfo {
    fp_PtrArray arr; // init with null
    // fp_PtrList* list;
    // int count;
} fp_mem__SizeClassInfo;
// size -> fp_mem__SizeClassInfo* map
static fp_Dict(UInt64, Ptr) fp_mem__sizeDict[1] = {};

#define STR(X) #X
#define SPOT(x, fn, fil, lin) fil ":" STR(lin) ": " x

// TODO: fp_mem_alloc should not be affected: disabling fp_heap_alloc merely
// makes fp_heap_alloc, fp_mem_heapfree identical to malloc,free.
// TODO: need 2 switches, one to turn off heap checker and one to just make
// everything fall back to malloc/free (no pool).

// API:
// fp_mem_alloc(str, size) -> str is usually var name. file/line/func are added
// fp_mem_dealloc(ptr, nelem) -> drop ptr with n elems (of size sizeof(*ptr))
// fp_mem_realloc(ptr, newsize) -> realloc the buf for ptr to a new size
// fp_mem_stats()
// fp_mem__alloc(size,rawstr) -> rawstr a string, file/line/func are not added
// fp_mem__dealloc(ptr, nbytes, rawstr) -> drop ptr with nbytes size

// #define FP_POOL_DISABLED
// #define NOHEAPTRACKER
#ifdef FP_POOL_DISABLED // --------------------------------------------------
#define fp_mem_alloc(nam, sz) fp_mem__alloc(sz, "")
void* fp_mem__heapmalloc(UInt64 size, const char* desc) { return malloc(size); }
#define fp_mem_heapfree free
void* fp_mem__alloc(UInt64 size, const char* desc)
{
    return fp_mem__heapmalloc(size, "");
}
UInt64 fp_mem_stats(bool heap) { return 0; }
#define fp_mem__dealloc(ptr, size, desc) fp_mem_heapfree(ptr)
#define fp_mem_dealloc(ptr, size) free(ptr)
#else // FP_POOL_DISABLED ---------------------------------------------------//
      // TODO: the compiler will call fp_mem__alloc directly with a custom built
      // string.
#define fp_mem_alloc(nam, sz)                                                  \
    fp_mem__alloc(sz, SPOT(nam, __func__, __FILE__, __LINE__))

static UInt64 fp_mem__heapTotal = 0;

typedef struct fp_mem__PtrInfo {
    UInt64 size : 63, heap : 1;
    const char* desc;
} fp_mem__PtrInfo;
MAKE_DICT(Ptr, fp_mem__PtrInfo)
// void* -> fp_mem__PtrInfo map
static fp_Dict(Ptr, fp_mem__PtrInfo) fp_mem__ptrDict[1];

// TODO: rename
typedef struct {
    UInt64 count, sum;
} fp_mem__Stat;

MAKE_DICT(CString, fp_mem__Stat)

// TODO: rename
fp_Dict(CString, fp_mem__Stat) fp_mem__allocStats[1];

int fp_mem__cmpsum(const void* a, const void* b)
{
    // For sorting in descending order of sum
    int ia = *(int*)a;
    int ib = *(int*)b;
    UInt64 va = fp_Dict_val(fp_mem__allocStats, ia).sum;
    UInt64 vb = fp_Dict_val(fp_mem__allocStats, ib).sum;
    return va > vb ? -1 : va == vb ? 0 : 1;
}

// void* fp_mem__heapmalloc(UInt64 size, const char* desc)
// {
//     EP;
//     void* ret = malloc(size);
//     // if (ret) {
//     //     int stat[1];
//     //     fp_mem__PtrInfo inf = { //
//     //         .size = size,
//     //         .heap = true,
//     //         .desc = desc
//     //     };
//     //     UInt32 p = fp_Dict_put(Ptr, fp_mem__PtrInfo)(fp_mem__ptrDict, ret,
//     stat);
//     //     fp_Dict_val(fp_mem__ptrDict, p) = inf;
//     //     fp_mem__heapTotal += size;
//     // }
//     EX;
//     return ret;
// }

void fp_mem__heapfree(void* ptr, const char* desc)
{
    EP;
    // int stat[1];
    int d = fp_Dict_get(Ptr, fp_mem__PtrInfo)(fp_mem__ptrDict, ptr);
    // use put so we can still access deleted items
    // printf("%d %d\n", d, fp_mem__ptrDict->nBuckets);
    // if (stat == 2) {

    if (d < fp_Dict_end(fp_mem__ptrDict)) {
        // else {
        if (not fp_Dict_val(fp_mem__ptrDict, d).heap) {
            printf("free non-heap pointer at %s\n", desc);
        } else {
            free(ptr);
            fp_mem__heapTotal -= fp_Dict_val(fp_mem__ptrDict, d).size;
            fp_Dict_delete(Ptr, fp_mem__PtrInfo)(fp_mem__ptrDict, d);
        }
    } else {
        printf("double free attempted at %s\n", desc);
    }
    EX
}

#define fp_mem_dealloc(x, n)                                                   \
    fp_mem__dealloc(                                                           \
        x, (n) * sizeof(*(x)), SPOT(#x, __func__, __FILE__, __LINE__))
#define fp_mem_heapfree(x)                                                     \
    fp_mem__heapfree(x, SPOT(#x, __func__, __FILE__, __LINE__))

// just put into freelist. why need desc?
// f+ in module fp.mem
// var sizeDict[UInt64] as SizeClassInfo = {} -- can be modif within module
// function dealloc(ptr as Ptr, size as UInt64, desc as CCharPtr)
//    var inf as SizeClassInfo =
//        getOrSet(sizeDict, key = size, value = SizeClassInfo())
//    insert(&inf.list, item = ptr)
//    inf.count += 1
// end function
static void fp_mem__dealloc(void* ptr, UInt64 size, const char* desc)
{
    EP;
    int stat[1] = {};
    int d = fp_Dict_put(UInt64, Ptr)(fp_mem__sizeDict, size, stat);

    // newly added, or got a previously deleted bucket so set new list
    if (*stat /* or not fp_Dict_val(fp_mem__sizeDict, d)*/) {
        fp_mem__SizeClassInfo* ptr = fp_new(fp_mem__SizeClassInfo);
        //*ptr =
        fp_Dict_val(fp_mem__sizeDict, d) = ptr;
    }
    fp_mem__SizeClassInfo* inf = fp_Dict_val(fp_mem__sizeDict, d);
    // if (inf->count>=32) { /* really free it if on heap using fp_mem_heapfree
    // */} else
    // {

    // inf->list = fp_PtrList_with_next(ptr, inf->list);
    fp_PtrArray_push(&inf->arr, ptr);

    // inf->count++;
#ifndef NOHEAPTRACKER
    fp_Dict_deleteByKey(Ptr, fp_mem__PtrInfo)(fp_mem__ptrDict, ptr);
#endif

    EX
}

// TODO: this func is not so great for strings, as it is "exact-fit". Freed
// pointers are reused only for objects of exactly the same size as the
// pointer's previous data. This is nearly useless for strings. They have their
// own string pool, you should probably have a fp_alloc_str that is best-fit.
// f+ fp.mem._alloc
void* fp_mem__alloc(UInt64 size, const char* desc)
// alloc from freelist, or pool or heap, in that order of preference
{
    EP;
#ifndef NOMEMPOOL
    int d = fp_Dict_get(UInt64, Ptr)(fp_mem__sizeDict, size);
    if (d < fp_Dict_end(fp_mem__sizeDict)) {
        fp_mem__SizeClassInfo* inf = fp_Dict_val(fp_mem__sizeDict, d);
        if (inf and inf->arr.used) {
            void* ret = fp_PtrArray_pop(&inf->arr); // inf->arr->item;
            // fp_mem_dealloc(inf->list, 1); // WHY?? -> to drop the listitem
            // inf->list = inf->list->next;
            // inf->count--;
            return ret;
        }
    }
    // see if it can fit in the pool, if yes, allocate from there.
    // if size is smallish <= N, force an allocation from the pool.
    // If current subpool doesn't have enough bytes, it'll calloc
    // a new subpool, you'll lose at most N-1 bytes.
    bool fromPool = size <= 256 or size <= (fp_gPool->cap - fp_gPool->used);
    void* ret = fromPool //
        ? fp_Pool_alloc(fp_gPool, size)
        // else fall back to the heap allocator.
        : malloc(size); // fp_mem__heapmalloc(size, desc);
    if (not fromPool) fp_mem__heapTotal += size;
#else
    void* ret = malloc(size);
#ifndef NOHEAPTOTAL
    fp_mem__heapTotal += size;
#endif
#endif

#ifndef NOHEAPTRACKER
    if (ret) {
        int stat[1];
        fp_mem__PtrInfo inf = { //
            .size = size,
            .heap = not fromPool,
            .desc = desc
        };
        UInt32 p
            = fp_Dict_put(Ptr, fp_mem__PtrInfo)(fp_mem__ptrDict, ret, stat);
        fp_Dict_val(fp_mem__ptrDict, p) = inf;
    }
#endif

    EX;
    return ret;
}

// f+ fp.mem.stats()
UInt64 fp_mem_stats(bool heap)
{
    EP;
    int i = 0;
    UInt64 sum = 0;
    if (fp_Dict_size(fp_mem__ptrDict)) {
        int stat[1];

        fp_Dict_foreach(fp_mem__ptrDict, Ptr key, fp_mem__PtrInfo val, {
            if (heap != val.heap) continue;
            int d = fp_Dict_put(CString, fp_mem__Stat)(
                fp_mem__allocStats, val.desc, stat);
            if (*stat) {
                fp_Dict_val(fp_mem__allocStats, d).sum = 0;
                fp_Dict_val(fp_mem__allocStats, d).count = 0;
            }
            fp_Dict_val(fp_mem__allocStats, d).sum += val.size;
            fp_Dict_val(fp_mem__allocStats, d).count++;
            sum += val.size;
        });

        if (fp_mem__allocStats->size) {
            int i = 0, *indxs = malloc(fp_mem__allocStats->size * sizeof(int));
            fp_Dict_foreach(fp_mem__allocStats, CString key, fp_mem__Stat val,
                { indxs[i++] = _i_; });
            qsort(indxs, fp_mem__allocStats->size, sizeof(int), fp_mem__cmpsum);
            printf("\n%s STATISTICS\n", heap ? "HEAP" : "POOL");
            puts("-------------------------------------------------------------"
                 "-");
            printf("Allocations | Total Size (B) | Variable and Location\n");
            puts("-------------------------------------------------------------"
                 "-");
            for (i = 0; i < fp_mem__allocStats->size; i++) {
                CString key = fp_mem__allocStats->keys[indxs[i]];
                fp_mem__Stat val = fp_mem__allocStats->vals[indxs[i]];
                printf("%11llu | %14llu | %s\n", val.count, val.sum, key);
            }
            UInt64 rss = heap ? fp_mem__heapTotal : fp_gPool->usedTotal;
            // if (sum) {
            puts("---------------------------------------------------------"
                 "-----");
            // todo: this should be heap total or pool total resp. depending on
            // the arg
            printf("Leaked %llu bytes (%.2f%% of %s total %llu "
                   "bytes)\n",
                sum, sum * 100.0 / rss, heap ? "heap" : "pool", rss);
            // }
            fp_Dict_clear(CString, fp_mem__Stat)(fp_mem__allocStats);
        } else {
            puts("-- no leaks.");
        }
    }

    EX;
    return sum;
}

#endif // FP_POOL_DISABLED --------------------------------------------------

int main()
{
    EP;
    double* d = fp_mem_alloc("d", 8 * sizeof(double));
    double* d3 = fp_mem_alloc("d3", 2 * sizeof(double));
    // fp_mem_dealloc(d3, 2);

    double* d2 = fp_mem_alloc("d2", 32 * sizeof(double));
    double *e, *ex;
    double sum = 0.0;
    for (int i = 0; i < 10000000; i++) {
        // TODO: in such a tight and long loop, if the subpool is close to the
        // end around the start of the loop, most of the allocations will go to
        // the heap -> a new subpool will not be created at all if size > 256,
        // and 1000s of allocs can fall through to malloc. So instead of this
        // 256 threshold you should, when you see subpool remainder not enough
        // for the request, make a fake allocation that covers the subpool
        // remainder, and alloc a new subpool anyway. direct fallthrough to
        // malloc should be only for requests that are really large.
        // to see the difference, change 256 to 257 below and see the effect.
        e = fp_mem_alloc("e", 32 * sizeof(double));
        ex = fp_mem_alloc("ex", 32 * sizeof(double));
        for (int j = 0; j < 32; j++) sum += e[j] + ex[j];
        // really large requests would be those larger than the next upcoming
        // subpool size.
        // printf("%d. %p\n", i, e);
        //...
        // if (i > 23)
        // fp_mem_dealloc(e, 32);
        // fp_mem_dealloc(ex, 32);
        // fp_mem_heapfree(e);
        // ex = fp_mem_alloc("ex as String", 14 * i * sizeof(char));
        // }
        // nopool: 3.5  alloc/free , 4.7  alloc/nofree
        // pool: 2 , 4.10
        // tracker: 3 , 1m45s
    }
    printf("%f", sum);

    // no pool -> 5.845s O3 S, 4.550 L
    // pool + tracking -> 3.415 O3 S, 4.624 L
    // pool no tracking -> 3.194 O3 S, 4.447 L
    // stack storage -> 2.502s S, L

    // fp_mem_dealloc(d, 8);
    fp_mem_dealloc(d3, 2);
    // fp_mem_dealloc(d2, 32);
    // fp_mem_heapfree(d3);
    // fp_mem_heapfree(d);
    // fp_mem_heapfree(d2);
    // typeof(*d) dc = d[0];
    // printf("%s", typeof(*d));
    // WHY REPORT? JUST WALK THE LIST AND FREE THEM BEFORE EXIT.
    // REPORT IS ONLY USEFUL TO SEE WHAT COULD HAVE BEEN RELEASED WHILE
    // THE PROGRAM WAS RUNNING, TO GIVE BACK MEM to the system.
    // SO DON'T CALL IT "LEAKED", JUST "LEFT OVER"

    fp_mem_stats(true);
    fp_mem_stats(false);

    fp_Dict_foreach(fp_mem__sizeDict, UInt64 key, fp_mem__SizeClassInfo * inf,
        {
            printf("size class %llu B -> %d ptrs, total %u B / actual %u B\n",
                key, inf->arr.used, inf->arr.used * 8, inf->arr.cap * 8);
        })

        printf("%d x %zu B -> %zu B SizeClassInfo\n",
            fp_mem__SizeClassInfo_allocTotal, sizeof(fp_mem__SizeClassInfo),
            fp_mem__SizeClassInfo_allocTotal * sizeof(fp_mem__SizeClassInfo));

    printf("%d x %zu B -> %zu B fp_PtrList\n", fp_PtrList_allocTotal,
        sizeof(fp_PtrList), fp_PtrList_allocTotal * sizeof(fp_PtrList));

    EX
}
#ifndef CHECKSTD
#define CHECKSTD

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h> /* sysconf(3) */

#ifdef WINDOWS
#include <windows.h>
#endif

#ifdef WINDOWS
size_t sys_pageSize()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}
#else
size_t sys_pageSize()
{
    return sysconf(_SC_PAGESIZE); /* _SC_PAGE_SIZE is OK too. */
}
#endif
#include "cycle.h"

// FIXME
#define _Pool_alloc_(x, y) malloc(y)

#define GB *1024 MB
#define MB *1024 KB
#define KB *1024UL

#define Array1D(x) Array1D

#include <sys/resource.h>
static size_t sys_stackSize()
{
    struct rlimit limit;
    getrlimit(RLIMIT_STACK, &limit);
    return limit.rlim_cur; //, limit.rlim_max);
}

#ifndef NONULLCHECK
// this is only for idents, not for funcs etc.: those will get evaluated
// twice if you use DEREF. Have to find another way for them (e.g.
// a.pop().xyz ) luckily if we stay away from methods and chaining we should
// be fine. P is for pointer to something, S is string, N is number (default
// values)
#define DEREF1(x, y) (x ? x->y : NULL)
#define DEREF2(x, y, z) (x && x->y ? x->y->z : NULL)
#define DEREF3(x, y, z, a) (x && x->y && x->y->z ? x->y->z->a : NULL)
#define DEREF4(x, y, z, a, b)                                              \
    (x && x->y && x->y->z && x->y->z->a ? x->y->z->a->b : NULL)

#else
// fast path, all guns blazing, no protection
#define DEREF1(x, y) x->y
#define DEREF2(x, y, z) x->y->z
#define DEREF3(x, y, z, a) x->y->z->a
#define DEREF4(x, y, z, a, b) x->y->z->a->b

#endif

#define BTLIMIT 5
#define ERROR_TRACE (char*)0xFFFFFFFFFFFFFFFF
#define DOBACKTRACE                                                        \
    {                                                                      \
        err_ = ERROR_TRACE;                                                \
        goto done;                                                         \
    }
// ulimit -s is 8192KB on Life, STACKMAX 8184 is last until which
// stackoverflow check works after that segfault
// #define STACKMAX 8160 KB // max stack to use. Get it from rusage etc.
static size_t STACKMAX;

typedef int Int;
typedef int Scalar;
typedef char** Strs;
typedef char* String;
#define DF_

// output funcs: print -> normal print, debug -> only prints (to stderr) in
// debug mode, error -> print to stderr, fatal -> print to stderr and exit
#define print printf

static void start(const Strs args
#ifdef DEBUG
    ,
    const char* callsite_
#endif
);

static const char* stackstart;
static int stackdepth = 0;
static int stackOverflowDepthThreshold = 0;

static const char* err_ = NULL;

int main(int argc, char* argv[])
{
    // const char* nul = NULL;
    // const char* err_ = NULL;

    ticks t0 = getticks();

    stackstart = (char*)&argc;
    STACKMAX = sys_stackSize() - 8192;

    start(NULL
#ifdef DEBUG
        ,
        "start(args)"
#endif
    );

    double dt = elapsed(getticks(), t0) / 1e9;
    if (err_ == ERROR_TRACE) {
        printf("[%.3fs] Aborted due to an unhandled error.\n", dt);
#ifndef DEBUG
        printf("(run in debug mode to see a backtrace)\n");
#endif
    } else if (err_ == NULL) {
        printf("[%.3fs] Completed successfully.\n", dt);
    }

    return err_ ? 1 : 0;
}

#endif // CHECKSTD
#ifndef CHECKSTD
#define CHECKSTD

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <unistd.h> /* sysconf(3) */

#include "chstd.h"

#ifdef WINDOWS
#include <windows.h>
#endif

size_t sys_pageSize()
{
#ifdef WINDOWS
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

#include "cycle.h"

// FIXME
#define _Pool_alloc_(x, y) malloc(y)

#define GB *1024 MB
#define MB *1024 KB
#define KB *1024UL

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

#define PROP(var, propname, type) JOIN_(type, propname)(var)
#define SETPROP(var, propname, type) JOIN3_(type, set, propname)(var)
// a.x becomes AType_x(a),  a.x = y becomes AType_set_x(a, y)

#define _btLimit_ 5
#define ERROR_TRACE (char*)0xFFFFFFFFFFFFFFFF
#define DOBACKTRACE                                                        \
    {                                                                      \
        _err_ = ERROR_TRACE;                                               \
        goto done;                                                         \
    }

static size_t _scSize_; // size of stack
static const char* _scStart_; // start of stack, set in main()
static int _scDepth_ = 0; // current depth, updated in each function
static int _scPrintAbove_ = 0; // used for truncating long backtraces
#ifdef DEBUG
#define STACKDEPTH_UP _scDepth_++;
#define STACKDEPTH_DOWN _scDepth_--;
#else
#define STACKDEPTH_UP
#define STACKDEPTH_DOWN
#endif
// what is the point of a separate NOSTACKCHECK option if release mode
// doesn't track ANY info (stack depth, function name etc.) other than
// showing "stack overflow" instead of "segmentation fault".

typedef int Int;
typedef int Scalar;
typedef char** Strs;
typedef char* String;
#define DEFAULT_VALUE
#define SArray(x) x[]

// output funcs: print -> normal print, debug -> only prints (to stderr) in
// debug mode, error -> print to stderr, fatal -> print to stderr and exit
#define print printf

static void start(const Strs args
#ifdef DEBUG
    ,
    const char* callsite_
#endif
);

static const char* _err_ = NULL;

int main(int argc, char* argv[])
{
    // const char* nul = NULL;
    // const char* _err_ = NULL;

    ticks t0 = getticks();

    _scStart_ = (char*)&argc;
    _scSize_ = sys_stackSize() - 8192;

    start(NULL
#ifdef DEBUG
        ,
        "start\033[0m"
#endif
    );

    double dt = elapsed(getticks(), t0) / 1e9;
    if (_err_ == ERROR_TRACE) {
        printf("[%.3fs] Aborted due to an unhandled error.\n", dt);
#ifndef DEBUG
        printf("(run in debug mode to see a backtrace)\n");
#endif
    } else if (_err_ == NULL) {
        printf("[%.3fs] Completed successfully.\n", dt);
    }

    return _err_ ? 1 : 0;
}

#endif // CHECKSTD
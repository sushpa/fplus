#ifndef CHECKSTD
#define CHECKSTD

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "cycle.h"

#define GB *1024 MB
#define MB *1024 KB
#define KB *1024UL

#include <sys/resource.h>
static size_t getstacksize()
{
    struct rlimit limit;
    getrlimit(RLIMIT_STACK, &limit);
    return limit.rlim_cur; //, limit.rlim_max);
}

#define BTLIMIT 20
#define ERROR_TRACE (char*)0xFFFFFFFFFFFFFFFF
#define DOBACKTRACE                                                        \
    {                                                                      \
        *err_ = ERROR_TRACE;                                               \
        goto backtrace;                                                    \
    }
// ulimit -s is 8192KB on Life, STACKMAX 8184 is last until which
// stackoverflow check works after that segfault
// #define STACKMAX 8160 KB // max stack to use. Get it from rusage etc.
static size_t STACKMAX;

typedef int Int;
typedef int Scalar;
typedef char** Strs;
#define DF_
#define print printf

static void start(const Strs args,
#ifdef DEBUG
    const char* file_, const int line_,
#endif
    const char** err_);

static const char* stackstart;
static int stackdepth = 0;

int main(int argc, char* argv[])
{
    const char* nul = NULL;
    const char** err_ = &nul;

    ticks t0 = getticks();

    stackstart = (char*)&argc;
    STACKMAX = getstacksize() - 8192;

    start(NULL,
#ifdef DEBUG
        "nowhere", 0,
#endif
        err_);

    double dt = elapsed(getticks(), t0) / 1e9;
    if (*err_ == ERROR_TRACE) {
        printf("[%.3fs] Aborted due to an unhandled error.\n", dt);
#ifndef DEBUG
        printf("(run in debug mode to see a backtrace)\n");
#endif
    } else if (*err_ == NULL) {
        printf("[%.3fs] Completed successfully.\n", dt);
    }

    return *err_ ? 1 : 0;
}

#endif // CHECKSTD
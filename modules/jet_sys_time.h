/*
 * Copyright (c) 2016-2020, Yann Collet, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under both the BSD-style license (found in the
 * LICENSE file in the root directory of this source tree) and the GPLv2 (found
 * in the COPYING file in the root directory of this source tree).
 * You may select, at your option, one of the above-listed licenses.
 */

/*-****************************************
 *  Dependencies
 ******************************************/
#include <time.h> /* clock_t, clock, CLOCKS_PER_SEC */

/*-****************************************
 *  Local Types
 ******************************************/

#if !defined(__VMS)                                                            \
    && (defined(__cplusplus)                                                   \
        || (defined(__STDC_VERSION__)                                          \
            && (__STDC_VERSION__ >= 199901L) /* C99 */))
#include <stdint.h>
typedef uint64_t PreciseTime; /* Precise Time */
#else
typedef unsigned long long
    PreciseTime; /* does not support compilers without long long support */
#endif

/*-****************************************
 *  Time functions
 ******************************************/
#if defined(_WIN32) /* Windows */

#include <windows.h> /* LARGE_INTEGER */
typedef LARGE_INTEGER jet_sys_time_Time;
#define jet_sys_time_TIME_INITIALIZER                                          \
    {                                                                          \
        {                                                                      \
            0, 0                                                               \
        }                                                                      \
    }

#elif defined(__APPLE__) && defined(__MACH__)

#include <mach/mach_time.h>
typedef PreciseTime jet_sys_time_Time;
#define jet_sys_time_TIME_INITIALIZER 0

/* C11 requires timespec_get, but FreeBSD 11 lacks it, while still claiming C11
   compliance. Android also lacks it but does define TIME_UTC. */
#elif (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) /* C11 */)   \
    && defined(TIME_UTC) && !defined(__ANDROID__)

typedef struct timespec jet_sys_time_Time;
#define jet_sys_time_TIME_INITIALIZER                                          \
    {                                                                          \
        0, 0                                                                   \
    }

#else /* relies on standard C90 (note : clock_t measurements can be wrong when \
         using multi-threading) */

typedef clock_t jet_sys_time_Time;
#define jet_sys_time_TIME_INITIALIZER 0

#endif

jet_sys_time_Time jet_sys_time_getTime(void);
PreciseTime jet_sys_time_getSpanTimeMicro(
    jet_sys_time_Time clockStart, jet_sys_time_Time clockEnd);
PreciseTime jet_sys_time_getSpanTimeNano(
    jet_sys_time_Time clockStart, jet_sys_time_Time clockEnd);

#define SEC_TO_MICRO ((PreciseTime)1000000)
PreciseTime jet_sys_time_clockSpanMicro(jet_sys_time_Time clockStart);
PreciseTime jet_sys_time_clockSpanNano(jet_sys_time_Time clockStart);

void jet_sys_time_waitForNextTick(void);

/*-****************************************
 *  Time functions
 ******************************************/

#if defined(_WIN32) /* Windows */

#include <stdlib.h> /* abort */
#include <stdio.h> /* perror */

jet_sys_time_Time jet_sys_time_getTime(void)
{
    jet_sys_time_Time x;
    QueryPerformanceCounter(&x);
    return x;
}

PreciseTime jet_sys_time_getSpanTimeMicro(
    jet_sys_time_Time clockStart, jet_sys_time_Time clockEnd)
{
    static LARGE_INTEGER ticksPerSecond;
    static int init = 0;
    if (!init) {
        if (!QueryPerformanceFrequency(&ticksPerSecond)) {
            perror("timefn::QueryPerformanceFrequency");
            abort();
        }
        init = 1;
    }
    return 1000000ULL * (clockEnd.QuadPart - clockStart.QuadPart)
        / ticksPerSecond.QuadPart;
}

PreciseTime jet_sys_time_getSpanTimeNano(
    jet_sys_time_Time clockStart, jet_sys_time_Time clockEnd)
{
    static LARGE_INTEGER ticksPerSecond;
    static int init = 0;
    if (!init) {
        if (!QueryPerformanceFrequency(&ticksPerSecond)) {
            perror("timefn::QueryPerformanceFrequency");
            abort();
        }
        init = 1;
    }
    return 1000000000ULL * (clockEnd.QuadPart - clockStart.QuadPart)
        / ticksPerSecond.QuadPart;
}

#elif defined(__APPLE__) && defined(__MACH__)

jet_sys_time_Time jet_sys_time_getTime(void) { return mach_absolute_time(); }

PreciseTime jet_sys_time_getSpanTimeMicro(
    jet_sys_time_Time clockStart, jet_sys_time_Time clockEnd)
{
    static mach_timebase_info_data_t rate;
    static int init = 0;
    if (!init) {
        mach_timebase_info(&rate);
        init = 1;
    }
    return (((clockEnd - clockStart) * (PreciseTime)rate.numer)
               / ((PreciseTime)rate.denom))
        / 1000ULL;
}

PreciseTime jet_sys_time_getSpanTimeNano(
    jet_sys_time_Time clockStart, jet_sys_time_Time clockEnd)
{
    static mach_timebase_info_data_t rate;
    static int init = 0;
    if (!init) {
        mach_timebase_info(&rate);
        init = 1;
    }
    return ((clockEnd - clockStart) * (PreciseTime)rate.numer)
        / ((PreciseTime)rate.denom);
}

/* C11 requires timespec_get, but FreeBSD 11 lacks it, while still claiming C11
   compliance. Android also lacks it but does define TIME_UTC. */
#elif (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L) /* C11 */)   \
    && defined(TIME_UTC) && !defined(__ANDROID__)

#include <stdlib.h> /* abort */
#include <stdio.h> /* perror */

jet_sys_time_Time jet_sys_time_getTime(void)
{
    /* time must be initialized, othersize it may fail msan test.
     * No good reason, likely a limitation of timespec_get() for some target */
    jet_sys_time_Time time = jet_sys_time_TIME_INITIALIZER;
    if (timespec_get(&time, TIME_UTC) != TIME_UTC) {
        perror("timefn::timespec_get");
        abort();
    }
    return time;
}

static jet_sys_time_Time jet_sys_time_getSpanTime(
    jet_sys_time_Time begin, jet_sys_time_Time end)
{
    jet_sys_time_Time diff;
    if (end.tv_nsec < begin.tv_nsec) {
        diff.tv_sec = (end.tv_sec - 1) - begin.tv_sec;
        diff.tv_nsec = (end.tv_nsec + 1000000000ULL) - begin.tv_nsec;
    } else {
        diff.tv_sec = end.tv_sec - begin.tv_sec;
        diff.tv_nsec = end.tv_nsec - begin.tv_nsec;
    }
    return diff;
}

PreciseTime jet_sys_time_getSpanTimeMicro(
    jet_sys_time_Time begin, jet_sys_time_Time end)
{
    jet_sys_time_Time const diff = jet_sys_time_getSpanTime(begin, end);
    PreciseTime micro = 0;
    micro += 1000000ULL * diff.tv_sec;
    micro += diff.tv_nsec / 1000ULL;
    return micro;
}

PreciseTime jet_sys_time_getSpanTimeNano(
    jet_sys_time_Time begin, jet_sys_time_Time end)
{
    jet_sys_time_Time const diff = jet_sys_time_getSpanTime(begin, end);
    PreciseTime nano = 0;
    nano += 1000000000ULL * diff.tv_sec;
    nano += diff.tv_nsec;
    return nano;
}

#else /* relies on standard C90 (note : clock_t measurements can be wrong when \
         using multi-threading) */

jet_sys_time_Time jet_sys_time_getTime(void) { return clock(); }
PreciseTime jet_sys_time_getSpanTimeMicro(
    jet_sys_time_Time clockStart, jet_sys_time_Time clockEnd)
{
    return 1000000ULL * (clockEnd - clockStart) / CLOCKS_PER_SEC;
}
PreciseTime jet_sys_time_getSpanTimeNano(
    jet_sys_time_Time clockStart, jet_sys_time_Time clockEnd)
{
    return 1000000000ULL * (clockEnd - clockStart) / CLOCKS_PER_SEC;
}

#endif

/* returns time span in microseconds */
PreciseTime jet_sys_time_clockSpanMicro(jet_sys_time_Time clockStart)
{
    jet_sys_time_Time const clockEnd = jet_sys_time_getTime();
    return jet_sys_time_getSpanTimeMicro(clockStart, clockEnd);
}

/* returns time span in microseconds */
PreciseTime jet_sys_time_clockSpanNano(jet_sys_time_Time clockStart)
{
    jet_sys_time_Time const clockEnd = jet_sys_time_getTime();
    return jet_sys_time_getSpanTimeNano(clockStart, clockEnd);
}

void jet_sys_time_waitForNextTick(void)
{
    jet_sys_time_Time const clockStart = jet_sys_time_getTime();
    jet_sys_time_Time clockEnd;
    do {
        clockEnd = jet_sys_time_getTime();
    } while (jet_sys_time_getSpanTimeNano(clockStart, clockEnd) == 0);
}
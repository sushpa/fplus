#include <stdio.h>
#include <time.h>

// typedef struct {
//     int y, tz;
//     unsigned int M, d, h, m, s, wd, dst,
//     _reserved_;
// } DateTime;

typedef struct {
    int y, M, d, h, m, s;
} Duration;

typedef struct {
    int y : 26, tz : 6;
    unsigned int M : 4, d : 5, h : 5, m : 6, s : 6, wd : 3, dst : 2,
        _reserved_ : 1;
} DateTime;

// This works only for the 8B DateTime structure
int DateTime_equal(DateTime a, DateTime b)
{
    int64_t *aa = &a, *bb = &b;
    return *aa == *bb;
}

// typedef struct {
//     int y;
//     int M : 5, d : 6, h : 6, m : 7, s : 7,
//         _reserved_ : 1;
// } Duration;

static const int saz = sizeof(Duration);
static const int sz = sizeof(DateTime);
// static const char* const weekDayName(int wd)
// {
//     switch (wd) {
//     case 0:
//         return "Sun";
//     case 1:
//         return "Mon";
//     case 2:
//         return "Tue";
//     case 3:
//         return "Wed";
//     case 4:
//         return "Thu";
//     case 5:
//         return "Fri";
//     case 6:
//         return "Sat";
//     }
//     return "";
// }
static const char* const weekDayName[]
    = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

int monthdays(int M, int y)
{
    switch (M % 12) {
    case 1:
        return 31;
    case 2:
        return 28 + !(y % 4);
    case 3:
        return 31;
    case 4:
        return 30;
    case 5:
        return 31;
    case 6:
        return 30;
    case 7:
        return 31;
    case 8:
        return 31;
    case 9:
        return 30;
    case 10:
        return 31;
    case 11:
        return 30;
    case 12:
    case 0:
        return 31;
    }
    return 0;
}

// Duration mkDuration(int y, int M, int d, int h, int m,
// int s)

static const char* const tzLabels_[] = { "-1200", "-1130", "-1100", "-1030",
    "-1000", "-0930", "-0900", "-0830", "-0800", "-0730", "-0700", "-0630",
    "-0600", "-0530", "-0500", "-0430", "-0400", "-0330", "-0300", "-0230",
    "-0200", "-0130", "-0100", "-0030", "", "+0030", "+0100", "+0130", "+0200",
    "+0230", "+0300", "+0330", "+0400", "+0430", "+0500", "+0530", "+0600",
    "+0630", "+0700", "+0730", "+0800", "+0830", "+0900", "+0930", "+1000",
    "+1030", "+1100", "+1130", "+1200" };
static const char* const* const tzLabels = tzLabels_ + 24;
// you can index this with [-24 .. +24]

void printDate(DateTime base)
{
    printf("DateTime %s %04d-%02d-%02d %02d:%02d:%02d %s\n",
        weekDayName[base.wd], base.y, base.M, base.d, base.h, base.m, base.s,
        tzLabels[base.tz]);

    struct tm t = {
        .tm_year = base.y - 1900,
        .tm_mon = base.M - 1,
        .tm_mday = base.d,
        .tm_hour = base.h,
        .tm_min = base.m,
        .tm_sec = base.s,
        .tm_isdst = base.dst,
        .tm_wday = base.wd,
        .tm_gmtoff = base.tz * 1800 // 30-min intervals
    };
    char buf[64];
    buf[63] = 0;
    strftime(buf, 63, "%c (Z%z: %Z)", &t);
    // printf("%s", buf);
}

void printDuration(Duration dur)
{
    printf("Duration %dy %dm %dd %dh %dm %ds\n", dur.y, dur.M, dur.d, dur.h,
        dur.m, dur.s);
}

DateTime dateAddN(DateTime base, Duration diff)
{
    long long sec = //
        diff.s
        + 60
            * (diff.m
                + 60
                    * (diff.h + 24 * (diff.d + 365 * (diff.y) + (diff.y / 4))));
    for (int i = 0; i < diff.M; i++) {
    }
}

DateTime dateAdd(DateTime base, Duration diff)
{
    if (diff.s >= 60) {
        diff.m += diff.s / 60;
        diff.s %= 60;
    }
    if (diff.m >= 60) {
        diff.h += diff.m / 60;
        diff.m %= 60;
    }
    if (diff.h >= 24) {
        diff.d += diff.h / 24;
        diff.h %= 24;
    }

    int mdays = monthdays(base.M + diff.M, base.y + diff.y);
    while (diff.d >= mdays) {
        diff.M += 1;
        mdays = monthdays(base.M + diff.M, base.y + diff.y);
        diff.d -= mdays;
    }

    if (diff.M >= 12) {
        diff.y += diff.M / 12;
        diff.M %= 12;
    }
    // printDuration(diff);

    int s = base.s + diff.s;

    int min = base.m + diff.m + (s >= 60);
    if (s >= 60) s -= 60;

    int h = base.h + diff.h + (min >= 60);
    if (min >= 60) min -= 60;

    int d = base.d + diff.d + (h >= 24);
    if (h >= 24) h -= 24;

    // mdays = monthdays(base.M + diff.M, base.y + diff.y);
    int M = base.M + diff.M + (d > mdays);
    if (d > mdays) d -= mdays;

    // int M = base.M + diff.M; //+ (d > mdays);
    // if (d > mdays) d = mdays;

    int y = base.y + diff.y + (M >= 12);
    int extradays = (diff.y + (M >= 12)) / 4;
    if (M >= 12) M -= 12;

    // if (extradays && diff.d) {
    //     // because you should move the d ahead only if
    //     // d-delta was specified. Perhaps also h-delta?
    //     d += extradays;
    //     // mdays = ...;
    //     M += (d >= mdays);
    //     if (d >= mdays) d -= mdays;
    //     y += (M >= 12);
    //     if (M >= 12) M -= 12;
    // }

    DateTime ret = {
        //
        .y = y,
        .M = M,
        .d = d,
        .h = h,
        .m = min,
        .s = s,
    };

    return ret;
}

DateTime now()
{
    time_t tmt[] = { time(NULL) };
    struct tm* tmnow = localtime(tmt);
    int s = sizeof(struct tm);
    // printf("%s %ld\n", tmnow->tm_zone, tmnow->tm_gmtoff);
    // long tz = tmnow->tm_gmtoff / 1800;

    // struct tm is anyway static inernal storage in localtime() which will be
    // overwritten, so it is not a bad idea to have our own (much smaller)
    // struct instead of just wrapping a struct tm
    return (DateTime) {
        .y = tmnow->tm_year + 1900,
        .M = tmnow->tm_mon + 1,
        .d = tmnow->tm_mday,
        .h = tmnow->tm_hour,
        .m = tmnow->tm_min,
        .s = tmnow->tm_sec,
        .dst = tmnow->tm_isdst ? (tmnow->tm_isdst > 0 ? 1 : -1) : 0,
        .wd = tmnow->tm_wday,
        .tz = tmnow->tm_gmtoff / 1800 // 30-min intervals
    };

    // return strftime
}

#define TEST_DATEADD(durn, exp)                                                \
    dur = durn;                                                                \
    result = dateAdd(base, dur);                                               \
    expected = exp;                                                            \
    if (!DateTime_equal(result, expected)) {                                   \
        printf("\n--- assertion failed:\n");                                   \
        printf("base ");                                                       \
        printDate(base);                                                       \
        printf("dur  ");                                                       \
        printDuration(dur);                                                    \
        printf("need ");                                                       \
        printDate(expected);                                                   \
        printf("got  ");                                                       \
        printDate(result);                                                     \
    }

int main()
{
    DateTime base = { //
        .y = 2016,
        .M = 5,
        .d = 27,
        .h = 12,
        .m = 15,
        .s = 40
    };

    Duration dur = { .y = 4, .h = 48, .d = 363 };
    // printDuration(dur);
    DateTime next = dateAdd(base, dur);

    // printDate(base);
    // printDate(next);
    printDate(now());
    DateTime result, expected;

    base = (DateTime) { .y = 2020, .M = 1, .d = 1 };
    TEST_DATEADD(
        ((Duration) { .y = 1 }), ((DateTime) { .y = 2021, .M = 1, .d = 1 }))
    TEST_DATEADD(
        ((Duration) { .M = 1 }), ((DateTime) { .y = 2020, .M = 2, .d = 1 }))
    TEST_DATEADD(
        ((Duration) { .d = 1 }), ((DateTime) { .y = 2020, .M = 1, .d = 2 }))
    TEST_DATEADD(((Duration) { .h = 1 }),
        ((DateTime) { .y = 2020, .M = 1, .d = 1, .h = 1 }))
    TEST_DATEADD(((Duration) { .m = 1 }),
        ((DateTime) { .y = 2020, .M = 1, .d = 1, .m = 1 }))
    TEST_DATEADD(((Duration) { .s = 1 }),
        ((DateTime) { .y = 2020, .M = 1, .d = 1, .s = 1 }))

    base = (DateTime) { .y = 2020, .M = 2, .d = 28 };
    TEST_DATEADD(
        ((Duration) { .d = 1 }), ((DateTime) { .y = 2020, .M = 2, .d = 29 }))
    TEST_DATEADD(
        ((Duration) { .M = 1 }), ((DateTime) { .y = 2020, .M = 3, .d = 28 }))

    base = (DateTime) { .y = 2020, .M = 1, .d = 30 };
    TEST_DATEADD(
        ((Duration) { .M = 1 }), ((DateTime) { .y = 2020, .M = 2, .d = 29 }))

    base = (DateTime) { .y = 2020, .M = 1, .d = 30 };
    TEST_DATEADD(((Duration) { .M = 1, .d = 5 }),
        ((DateTime) { .y = 2020, .M = 3, .d = 1 }))

    base = (DateTime) { .y = 2020, .M = 5, .d = 31 };
    TEST_DATEADD(
        ((Duration) { .M = 1 }), ((DateTime) { .y = 2020, .M = 6, .d = 30 }))
}

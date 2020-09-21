//#include <stdio.h>
#include <time.h>
#include <assert.h>
#include "jet_base.h"

typedef struct {
    int y;
    unsigned char tz, M, d, h, m, s, wd, dst;
} DateTime;

typedef struct {
    int y, M, d, h, m, s;
} Duration;

// typedef struct {
//    int y : 26, tz : 6;
//    unsigned int M : 4, d : 5, h : 5, m : 6, s : 6, wd : 3, dst : 2,
//        _reserved_ : 1;
//} DateTime;

// This works only for the 8B DateTime structure

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
    return 1000000;
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

struct tm DateTime_toTM(DateTime base)
{
    struct tm tmret = {
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
    return tmret; // mktime(&tmret);
}

DateTime DateTime_fromTM(struct tm* tmret)
{
    // tmret->tm_isdst = -1;
    // mktime(tmret);
    return (DateTime) {
        .y = tmret->tm_year + 1900,
        .M = tmret->tm_mon + 1,
        .d = tmret->tm_mday,
        .h = tmret->tm_hour,
        .m = tmret->tm_min,
        .s = tmret->tm_sec,
        .dst = tmret->tm_isdst ? (tmret->tm_isdst > 0 ? 1 : -1) : 0,
        .wd = tmret->tm_wday,
        .tz = tmret->tm_gmtoff / 1800 // 30-min intervals
    };
}

time_t DateTime_toUnix(DateTime base)
{
    struct tm t = DateTime_toTM(base);
    return mktime(&t);
}

DateTime DateTime_fromUnix(time_t t)
{
    struct tm* tmret = gmtime(&t);
    return DateTime_fromTM(tmret);
}

int DateTime_equal(DateTime a, DateTime b)
{
    // int64_t *aa = &a, *bb = &b;
    // return *aa == *bb;

    time_t ta = DateTime_toUnix(a); //+a.tz*1800;
    time_t tb = DateTime_toUnix(b); //+b.tz*1800;
    return ta == tb;
    //    return a.y == b.y && a.M == b.M && a.d == b.d && a.h == b.h && a.m ==
    //    b.m
    //    && a.s == b.s; //&& a.tz == b.tz;
}

void DateTime_print(DateTime base)
{
    printf("DateTime %s %04d-%02d-%02d %02d:%02d:%02d %s\n",
        weekDayName[base.wd], base.y, base.M, base.d, base.h, base.m, base.s,
        tzLabels[base.tz]);

    // struct tm t = DateTime_toTM(base);
    // // {
    // //     .tm_year = base.y - 1900,
    // //     .tm_mon = base.M - 1,
    // //     .tm_mday = base.d,
    // //     .tm_hour = base.h,
    // //     .tm_min = base.m,
    // //     .tm_sec = base.s,
    // //     .tm_isdst = base.dst,
    // //     .tm_wday = base.wd,
    // //     .tm_gmtoff = base.tz * 1800 // 30-min intervals
    // // };
    // char buf[64];
    // buf[63] = 0;
    // strftime(buf, 63, "%c (%Z)\n", &t);
    // printf("%s", buf);
}

void Duration_print(Duration dur)
{
    printf("Duration %dy %dM %dd %dh %dm %ds\n", dur.y, dur.M, dur.d, dur.h,
        dur.m, dur.s);
}

Duration Duration_new(
    int years, int months, int days, int hours, int minutes, int seconds)
{
    Duration diff = { .y = years,
        .M = months,
        .d = days,
        .h = hours,
        .m = minutes,
        .s = seconds };
    // if (diff.s >= 60) {
    diff.m += diff.s / 60;
    diff.s %= 60;
    // }
    // if (diff.m >= 60) {
    diff.h += diff.m / 60;
    diff.m %= 60;
    // }
    // if (diff.h >= 24) {
    diff.d += diff.h / 24;
    diff.h %= 24;
    // }

    return diff;
}

/*
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

    const int daysIn4Years = 365 * 4 + 1;
    if (diff.d >= daysIn4Years) {
        diff.y += 4 * (diff.d / daysIn4Years);
        diff.d -= daysIn4Years;
    }

    int ytmp = base.y;
    int daysIn1Year = 365 + !(ytmp % 4);
    while (diff.d >= daysIn1Year) {
        diff.y += 1;
        diff.d -= daysIn1Year;
        daysIn1Year = 365 + !(ytmp % 4);
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
    // Duration_print(diff);

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
*/

DateTime DateTime_new(int year, int month, int day)
{
    time_t tt = 0;
    struct tm tmt[1] = { {} };
    gmtime_r(&tt, tmt); // use gmtime to not care about timezones

    char buf[64];
    buf[63] = 0;
    // strftime(buf, 63, "%c (%Z)\n", tmt);
    // printf(".  %s", buf);

    tmt->tm_year = year - 1900;
    tmt->tm_mon = month - 1;
    tmt->tm_mday = day;
    tmt->tm_hour = tmt->tm_min = tmt->tm_sec = 0;
    tmt->tm_isdst = 0;

    tt = mktime(tmt);
    // strftime(buf, 63, "%c (%Z)\n", tmt);
    // printf(".. %s", buf);

    return DateTime_fromTM(tmt);
}

DateTime DateTime_newWithTime(
    int year, int month, int day, int hour, int minute, int second)
{
    DateTime t = DateTime_new(year, month, day);
    t.h = hour;
    t.m = minute;
    t.s = second;
    return t;
}

DateTime DateTime_add(DateTime base, Duration diff)
{

    // save orig base here
    DateTime orig = base;

    diff.y += diff.M / 12;
    diff.M %= 12;

    base.y += diff.y;
    base.M += diff.M;
    if (base.M > 12) {
        base.y += 1;
        base.M -= 12;
    }

    int mdays = monthdays(base.M, base.y);
    if (base.d > mdays) base.d = mdays;

    time_t dsec = //
        diff.s
        + 60
            * (diff.m
                + 60
                    * (diff.h
                        + 24 * (diff.d /*+ 365 * (diff.y) + (diff.y / 4)*/)));

    // int yy = base.y + diff.y;
    // int mm = base.M + diff.M;
    // if (mm > 12) {
    //     diff.y++;
    //     mm -= 12;
    // }
    // if (diff.y)
    //     dsec += 60 * 60 * 24
    //         * (base.y % 4 == 0 || (base.y % 4 + diff.y % 4 >= 4));
    // int month = base.M, year = base.y;
    // for (int i = 0; i < diff.M; i++) {
    //     int mdays = monthdays(month, year);
    //     dsec += mdays * 24 * 60 * 60;
    //     // year += !(++month % 12);
    //     month += 1;
    //     if (month > 12) {
    //         month -= 12;
    //         year += 1;
    //     }
    // }

    // struct tm t = DateTime_toTM(base);

    // time_t baset = mktime(&t);
    // // printf("  %15ld\n+ %15ld\n= %15ld\n", baset, dsec, baset + dsec);
    // time_t tmtret = baset + dsec;
    // struct tm* tmret = gmtime(&tmtret);

    time_t ttorig = DateTime_toUnix(orig) + orig.tz * 1800;
    time_t ttbase = DateTime_toUnix(base) + base.tz * 1800;
    time_t ttret = ttbase + dsec;

    DateTime ret = DateTime_fromUnix(ttret);
    //    ret.tz = base.tz;
    //    ret.dst = base.dst;

    //    ret.h += ret.tz / 2;
    //    ret.m += (ret.tz % 2) * 30;
    //    ret.d += ret.h / 24;
    //    ret.h %= 24;
    // ret.M += diff.M; // this is a special treatment
    // ret.M = mm;
    // ret.y = yy;
    // if (0&&diff.d>1){
    //    time_t ttleap=0;
    //   for (int yy = orig.y - (orig.y%4);yy<=ret.y;yy+=4) {
    //        time_t feb29 = DateTime_toUnix(DateTime_new(yy,2,29));
    //        if (feb29>=ttorig && feb29<=ttret) ttleap+=24*60*60;
    //    }
    //
    //    ttret +=ttleap;
    //     ret = DateTime_fromUnix(ttret);
    //}
    ret.tz = base.tz;
    ret.dst = base.dst;

    // TODO: correction for leap days: go over all leap days starting from 29/2
    // in orig.year and check if it falls within the interval [ orig ... new ].
    // if so add a day (check overflow, also for months if > 30 days added...)

    // snap
    // int mdays = monthdays(ret.M, ret.y);
    // if (ret.d > mdays) ret.d = mdays;

    return ret;
}

DateTime DateTime_now()
{
    // time_t tmt[] = { time(NULL) };
    // struct tm* tmnow = gmtime(tmt);
    // int s = sizeof(struct tm);
    // printf("%s %ld\n", tmnow->tm_zone, tmnow->tm_gmtoff);
    // long tz = tmnow->tm_gmtoff / 1800;

    // struct tm is anyway static inernal storage in gmtime() which will be
    // overwritten, so it is not a bad idea to have our own (much smaller)
    // struct instead of just wrapping a struct tm
    return DateTime_fromUnix(time(NULL));
    // {
    //     .y = tmnow->tm_year + 1900,
    //     .M = tmnow->tm_mon + 1,
    //     .d = tmnow->tm_mday,
    //     .h = tmnow->tm_hour,
    //     .m = tmnow->tm_min,
    //     .s = tmnow->tm_sec,
    //     .dst = tmnow->tm_isdst ? (tmnow->tm_isdst > 0 ? 1 : -1) : 0,
    //     .wd = tmnow->tm_wday,
    //     .tz = tmnow->tm_gmtoff / 1800 // 30-min intervals
    // };

    // return strftime
}

// #define TEST_DATEADD(durn, exp)                                                \
//     dur = durn;                                                                \
//     result = dateAdd(base, dur);                                               \
//     expected = exp;                                                            \
//     if (!DateTime_equal(result, expected)) {                                   \
//         printf("\n--- assertion failed:\n");                                   \
//         printf("base ");                                                       \
//         DateTime_print(base);                                                       \
//         printf("dur  ");                                                       \
//         Duration_print(dur);                                                    \
//         printf("need ");                                                       \
//         DateTime_print(expected);                                                   \
//         printf("got  ");                                                       \
//         DateTime_print(result);                                                     \
//     }

#define TEST_ADD(by, bm, bd, dy, dM, dd, ey, em, ed)                           \
    TEST_ADDT(by, bm, bd, dy, dM, dd, 0, 0, 0, ey, em, ed)

#define TEST_ADDT(by, bm, bd, dy, dM, dd, dh, dm, ds, ey, em, ed)              \
    lineno = __LINE__;                                                         \
    ntot++;                                                                    \
    dur = Duration_new(dy, dM, dd, dh, dm, ds);                                \
    base = DateTime_new(by, bm, bd);                                           \
    result = DateTime_add(base, dur);                                          \
    expected = DateTime_new(ey, em, ed);                                       \
    if (!DateTime_equal(result, expected)) {                                   \
        printf("\n--- test #%d failed at ./%s:%d:\n", ntot, __FILE__, lineno); \
        printf("dur  ");                                                       \
        Duration_print(dur);                                                   \
        printf("base ");                                                       \
        DateTime_print(base);                                                  \
        printf("need ");                                                       \
        DateTime_print(expected);                                              \
        printf("got  ");                                                       \
        DateTime_print(result);                                                \
        nfail++;                                                               \
    }
int maian()
{
    DateTime base; // = { //
    //     .y = 2016,
    //     .M = 5,
    //     .d = 27,
    //     .h = 12,
    //     .m = 15,
    //     .s = 40
    // };
    int lineno, ntot = 0, nfail = 0;
    Duration dur; //= { .y = 4, .h = 48, .d = 363 };
    // Duration_print(dur);
    // DateTime next = DateTime_add(base, dur);

    // DateTime_print(base);
    // DateTime_print(next);
    printf("now:  ");
    DateTime_print(DateTime_now());
    DateTime result, expected;

    // base = (DateTime) { .y = 2020, .M = 1, .d = 1 };
    TEST_ADD(2020, 1, 1, 1, 0, 0, 2021, 1, 1)
    TEST_ADD(2021, 1, 1, 1, 0, 0, 2022, 1, 1)
    TEST_ADD(2020, 1, 1, 0, 1, 0, 2020, 2, 1)
    TEST_ADD(2020, 1, 1, 0, 0, 1, 2020, 1, 2)

    TEST_ADD(2020, 2, 28, 0, 0, 1, 2020, 2, 29)
    TEST_ADD(2021, 2, 28, 0, 0, 1, 2021, 3, 1)

    TEST_ADD(2020, 2, 28, 0, 1, 0, 2020, 3, 28)
    TEST_ADD(2021, 2, 28, 0, 1, 0, 2021, 3, 28)

    TEST_ADD(2020, 1, 30, 0, 1, 0, 2020, 2, 29)
    TEST_ADD(2020, 1, 30, 0, 1, 5, 2020, 3, 5)
    TEST_ADD(2020, 3, 30, 0, 1, 5, 2020, 5, 5)
    TEST_ADD(2020, 4, 30, 0, 1, 5, 2020, 6, 4)

    TEST_ADD(2020, 1, 30, 0, 2, 0, 2020, 3, 30)

    TEST_ADD(2020, 5, 31, 0, 1, 0, 2020, 6, 30)

    TEST_ADD(2020, 1, 1, 0, 0, 390, 2021, 1, 25)
    TEST_ADD(2021, 1, 1, 0, 0, 390, 2022, 1, 26)

    TEST_ADD(2020, 1, 1, 0, 32, 390, 2023, 9, 26)
    TEST_ADD(2021, 1, 1, 0, 32, 390, 2024, 9, 25)

    TEST_ADD(2020, 2, 29, 1, 0, 0, 2021, 2, 28)
    TEST_ADDT(2020, 2, 29, 0, 0, 0, 0, 0, 365 * 24 * 60 * 60, 2021, 2, 28)

    // base = (DateTime) { .y = 2020, .M = 2, .d = 28 };
    // TEST_DATEADD(
    //     ((Duration) { .d = 1 }), ((DateTime) { .y = 2020, .M = 2, .d = 29 }))
    // TEST_DATEADD(
    //     ((Duration) { .M = 1 }), ((DateTime) { .y = 2020, .M = 3, .d = 28 }))

    // base = (DateTime) { .y = 2020, .M = 1, .d = 30 };
    // TEST_DATEADD(
    //     ((Duration) { .M = 1 }), ((DateTime) { .y = 2020, .M = 2, .d = 29 }))

    // base = (DateTime) { .y = 2020, .M = 1, .d = 30 };
    // TEST_DATEADD(((Duration) { .M = 1, .d = 5 }),
    //     ((DateTime) { .y = 2020, .M = 3, .d = 1 }))

    // base = (DateTime) { .y = 2020, .M = 5, .d = 31 };
    // TEST_DATEADD(
    //     ((Duration) { .M = 1 }), ((DateTime) { .y = 2020, .M = 6, .d = 30 }))
    if (nfail) printf("\n--- %d of %d tests failed.\n", nfail, ntot);
    return 0;
}

typedef union {
    unsigned int value;
    struct {
#ifdef BIGENDIAN
        unsigned char alpha, red, green, blue;
#else
        unsigned char blue, green, red, alpha;
#endif
    };
} Colour;
static_assert(sizeof(Colour) == 4, "");

Colour invert(Colour col)
{
    return (Colour) { .red = 255 - col.red,
        .green = 255 - col.green,
        .blue = 255 - col.blue,
        .alpha = col.alpha };
}

enum ColourBlendModes {
    ColourBlendModes_SoftLight,
    ColourBlendModes_DarkLight,
    ColourBlendModes_Multiply,
    ColourBlendModes_Dissolve,
    ColourBlendModes_Normal,
};

Colour blend(Colour col, Colour col2, float factor)
{
    return (Colour) { .red = factor * col.red + (1 - factor) * col2.red,
        .green = factor * col.green + (1 - factor) * col2.green,
        .blue = factor * col.blue + (1 - factor) * col2.blue,
        .alpha = col.alpha };
}

Colour darken(Colour col, float amount)
{
    col.red *= 1 - amount;
    col.green *= 1 - amount;
    col.blue *= 1 - amount;
    return col;
}

Colour lighten(Colour col, float amount)
{
    col.red += amount * (255 - col.red);
    col.green += amount * (255 - col.green);
    col.blue += amount * (255 - col.blue);
    return col;
}

Colour Colour_rgba(int r, int g, int b, int a)
{
    return (Colour) { .red = r, .green = g, .blue = b, .alpha = a };
}

Colour Colour_rgb(int r, int g, int b) { return Colour_rgba(r, g, b, 0); }
Colour Colour_new(unsigned int value)
{
    //    unsigned char *bytes = (unsigned char *)&value;
    //#ifdef BIGENDIAN
    //    return Colour_rgba(bytes[0],bytes[1], bytes[2], bytes[3]);
    //#else
    //    return Colour_rgba(bytes[3],bytes[2], bytes[1], bytes[0]);
    //#endif
    return (Colour) { .value = value };
}

void Colour_print(Colour col)
{
    printf("#%02x%02x%02x", col.red, col.green, col.blue);
    if (col.alpha) printf("%02x", col.alpha);
    puts("");
}

void Colour_printRGBA(Colour col)
{
    if (col.alpha)
        printf("rgba(%d,%d,%d,%d)\n", col.red, col.green, col.blue, col.alpha);
    else
        printf("rgb(%d,%d,%d)\n", col.red, col.green, col.blue);
}

typedef int YesOrNo;
#define no 0
#define yes 1

YesOrNo isDark(Colour col)
{
    return no; // NYI
}

enum SystemColours {
    SystemColours_window,
    SystemColours_text,
    SystemColours_background,
    SystemColours_highlight,
    SystemColours_accent,
    SystemColours_red,
    SystemColours_green,
    SystemColours_blue,
    SystemColours_yellow,
    SystemColours_purple,
    SystemColours_orange,
    SystemColours_systemred,
    SystemColours_systemgreen,
    SystemColours_systemblue,
    SystemColours_systemyellow,
    SystemColours_systempurple,
    SystemColours_systemorange
};

const Colour _systemColoursArray[] = {
    [SystemColours_window] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_text] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_background]
    = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_highlight] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_accent] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_red] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_green] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_blue] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_yellow] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_purple] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_orange] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_systemred] = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_systemgreen]
    = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_systemblue]
    = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_systemyellow]
    = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_systempurple]
    = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
    [SystemColours_systemorange]
    = { .red = 0, .green = 0, .blue = 0, .alpha = 0 },
};

Colour Colour_systemColour(enum SystemColours which)
{
    return _systemColoursArray[which];
}

enum KnownColours {
    KnownColours_indianRed,
    KnownColours_lightCoral,
    KnownColours_salmon,
    KnownColours_darkSalmon,
    KnownColours_lightSalmon,
    KnownColours_crimson,
    KnownColours_red,
    KnownColours_fireBrick,
    KnownColours_darkRed,
    KnownColours_pink,
    KnownColours_lightPink,
    KnownColours_hotPink,
    KnownColours_deepPink,
    KnownColours_mediumVioletRed,
    KnownColours_paleVioletRed,
    KnownColours_coral,
    KnownColours_tomato,
    KnownColours_orangeRed,
    KnownColours_darkOrange,
    KnownColours_orange,
    KnownColours_gold,
    KnownColours_yellow,
    KnownColours_lightYellow,
    KnownColours_lemonChiffon,
    KnownColours_lightGoldenrodYellow,
    KnownColours_papayaWhip,
    KnownColours_moccasin,
    KnownColours_peachPuff,
    KnownColours_paleGoldenrod,
    KnownColours_khaki,
    KnownColours_darkKhaki,
    KnownColours_lavender,
    KnownColours_thistle,
    KnownColours_plum,
    KnownColours_violet,
    KnownColours_orchid,
    KnownColours_fuchsia,
    KnownColours_magenta,
    KnownColours_mediumOrchid,
    KnownColours_mediumPurple,
    KnownColours_blueViolet,
    KnownColours_darkViolet,
    KnownColours_darkOrchid,
    KnownColours_darkMagenta,
    KnownColours_purple,
    KnownColours_rebeccaPurple,
    KnownColours_indigo,
    KnownColours_mediumSlateBlue,
    KnownColours_slateBlue,
    KnownColours_darkSlateBlue,
    KnownColours_greenYellow,
    KnownColours_chartreuse,
    KnownColours_lawnGreen,
    KnownColours_lime,
    KnownColours_limeGreen,
    KnownColours_paleGreen,
    KnownColours_lightGreen,
    KnownColours_mediumSpringGreen,
    KnownColours_springGreen,
    KnownColours_mediumSeaGreen,
    KnownColours_seaGreen,
    KnownColours_forestGreen,
    KnownColours_green,
    KnownColours_darkGreen,
    KnownColours_yellowGreen,
    KnownColours_oliveDrab,
    KnownColours_olive,
    KnownColours_darkOliveGreen,
    KnownColours_mediumAquamarine,
    KnownColours_darkSeaGreen,
    KnownColours_lightSeaGreen,
    KnownColours_darkCyan,
    KnownColours_teal,
    KnownColours_aqua,
    KnownColours_cyan,
    KnownColours_lightCyan,
    KnownColours_paleTurquoise,
    KnownColours_aquamarine,
    KnownColours_turquoise,
    KnownColours_mediumTurquoise,
    KnownColours_darkTurquoise,
    KnownColours_cadetBlue,
    KnownColours_steelBlue,
    KnownColours_lightSteelBlue,
    KnownColours_powderBlue,
    KnownColours_lightBlue,
    KnownColours_skyBlue,
    KnownColours_lightSkyBlue,
    KnownColours_deepSkyBlue,
    KnownColours_dodgerBlue,
    KnownColours_cornflowerBlue,
    KnownColours_royalBlue,
    KnownColours_blue,
    KnownColours_mediumBlue,
    KnownColours_darkBlue,
    KnownColours_navy,
    KnownColours_midnightBlue,
    KnownColours_cornsilk,
    KnownColours_blanchedAlmond,
    KnownColours_bisque,
    KnownColours_navajoWhite,
    KnownColours_wheat,
    KnownColours_burlyWood,
    KnownColours_tan,
    KnownColours_rosyBrown,
    KnownColours_sandyBrown,
    KnownColours_goldenrod,
    KnownColours_darkGoldenrod,
    KnownColours_peru,
    KnownColours_chocolate,
    KnownColours_saddleBrown,
    KnownColours_sienna,
    KnownColours_brown,
    KnownColours_maroon,
    KnownColours_white,
    KnownColours_snow,
    KnownColours_honeydew,
    KnownColours_mintCream,
    KnownColours_azure,
    KnownColours_aliceBlue,
    KnownColours_ghostWhite,
    KnownColours_whiteSmoke,
    KnownColours_seashell,
    KnownColours_beige,
    KnownColours_oldLace,
    KnownColours_floralWhite,
    KnownColours_ivory,
    KnownColours_antiqueWhite,
    KnownColours_linen,
    KnownColours_lavenderBlush,
    KnownColours_mistyRose,
    KnownColours_gainsboro,
    KnownColours_lightGray,
    KnownColours_silver,
    KnownColours_darkGray,
    KnownColours_gray,
    KnownColours_dimGray,
    KnownColours_lightSlateGray,
    KnownColours_slateGray,
    KnownColours_darkSlateGray,
    KnownColours_black
};

static const char* const KnownColours__names[]
    = { [KnownColours_indianRed] = "indianRed",
          [KnownColours_lightCoral] = "lightCoral",
          [KnownColours_salmon] = "salmon",
          [KnownColours_darkSalmon] = "darkSalmon",
          [KnownColours_lightSalmon] = "lightSalmon",
          [KnownColours_crimson] = "crimson",
          [KnownColours_red] = "red",
          [KnownColours_fireBrick] = "fireBrick",
          [KnownColours_darkRed] = "darkRed",
          [KnownColours_pink] = "pink",
          [KnownColours_lightPink] = "lightPink",
          [KnownColours_hotPink] = "hotPink",
          [KnownColours_deepPink] = "deepPink",
          [KnownColours_mediumVioletRed] = "mediumVioletRed",
          [KnownColours_paleVioletRed] = "paleVioletRed",
          [KnownColours_coral] = "coral",
          [KnownColours_tomato] = "tomato",
          [KnownColours_orangeRed] = "orangeRed",
          [KnownColours_darkOrange] = "darkOrange",
          [KnownColours_orange] = "orange",
          [KnownColours_gold] = "gold",
          [KnownColours_yellow] = "yellow",
          [KnownColours_lightYellow] = "lightYellow",
          [KnownColours_lemonChiffon] = "lemonChiffon",
          [KnownColours_lightGoldenrodYellow] = "lightGoldenrodYellow",
          [KnownColours_papayaWhip] = "papayaWhip",
          [KnownColours_moccasin] = "moccasin",
          [KnownColours_peachPuff] = "peachPuff",
          [KnownColours_paleGoldenrod] = "paleGoldenrod",
          [KnownColours_khaki] = "khaki",
          [KnownColours_darkKhaki] = "darkKhaki",
          [KnownColours_lavender] = "lavender",
          [KnownColours_thistle] = "thistle",
          [KnownColours_plum] = "plum",
          [KnownColours_violet] = "violet",
          [KnownColours_orchid] = "orchid",
          [KnownColours_fuchsia] = "fuchsia",
          [KnownColours_magenta] = "magenta",
          [KnownColours_mediumOrchid] = "mediumOrchid",
          [KnownColours_mediumPurple] = "mediumPurple",
          [KnownColours_blueViolet] = "blueViolet",
          [KnownColours_darkViolet] = "darkViolet",
          [KnownColours_darkOrchid] = "darkOrchid",
          [KnownColours_darkMagenta] = "darkMagenta",
          [KnownColours_purple] = "purple",
          [KnownColours_rebeccaPurple] = "rebeccaPurple",
          [KnownColours_indigo] = "indigo",
          [KnownColours_mediumSlateBlue] = "mediumSlateBlue",
          [KnownColours_slateBlue] = "slateBlue",
          [KnownColours_darkSlateBlue] = "darkSlateBlue",
          [KnownColours_greenYellow] = "greenYellow",
          [KnownColours_chartreuse] = "chartreuse",
          [KnownColours_lawnGreen] = "lawnGreen",
          [KnownColours_lime] = "lime",
          [KnownColours_limeGreen] = "limeGreen",
          [KnownColours_paleGreen] = "paleGreen",
          [KnownColours_lightGreen] = "lightGreen",
          [KnownColours_mediumSpringGreen] = "mediumSpringGreen",
          [KnownColours_springGreen] = "springGreen",
          [KnownColours_mediumSeaGreen] = "mediumSeaGreen",
          [KnownColours_seaGreen] = "seaGreen",
          [KnownColours_forestGreen] = "forestGreen",
          [KnownColours_green] = "green",
          [KnownColours_darkGreen] = "darkGreen",
          [KnownColours_yellowGreen] = "yellowGreen",
          [KnownColours_oliveDrab] = "oliveDrab",
          [KnownColours_olive] = "olive",
          [KnownColours_darkOliveGreen] = "darkOliveGreen",
          [KnownColours_mediumAquamarine] = "mediumAquamarine",
          [KnownColours_darkSeaGreen] = "darkSeaGreen",
          [KnownColours_lightSeaGreen] = "lightSeaGreen",
          [KnownColours_darkCyan] = "darkCyan",
          [KnownColours_teal] = "teal",
          [KnownColours_aqua] = "aqua",
          [KnownColours_cyan] = "cyan",
          [KnownColours_lightCyan] = "lightCyan",
          [KnownColours_paleTurquoise] = "paleTurquoise",
          [KnownColours_aquamarine] = "aquamarine",
          [KnownColours_turquoise] = "turquoise",
          [KnownColours_mediumTurquoise] = "mediumTurquoise",
          [KnownColours_darkTurquoise] = "darkTurquoise",
          [KnownColours_cadetBlue] = "cadetBlue",
          [KnownColours_steelBlue] = "steelBlue",
          [KnownColours_lightSteelBlue] = "lightSteelBlue",
          [KnownColours_powderBlue] = "powderBlue",
          [KnownColours_lightBlue] = "lightBlue",
          [KnownColours_skyBlue] = "skyBlue",
          [KnownColours_lightSkyBlue] = "lightSkyBlue",
          [KnownColours_deepSkyBlue] = "deepSkyBlue",
          [KnownColours_dodgerBlue] = "dodgerBlue",
          [KnownColours_cornflowerBlue] = "cornflowerBlue",
          [KnownColours_royalBlue] = "royalBlue",
          [KnownColours_blue] = "blue",
          [KnownColours_mediumBlue] = "mediumBlue",
          [KnownColours_darkBlue] = "darkBlue",
          [KnownColours_navy] = "navy",
          [KnownColours_midnightBlue] = "midnightBlue",
          [KnownColours_cornsilk] = "cornsilk",
          [KnownColours_blanchedAlmond] = "blanchedAlmond",
          [KnownColours_bisque] = "bisque",
          [KnownColours_navajoWhite] = "navajoWhite",
          [KnownColours_wheat] = "wheat",
          [KnownColours_burlyWood] = "burlyWood",
          [KnownColours_tan] = "tan",
          [KnownColours_rosyBrown] = "rosyBrown",
          [KnownColours_sandyBrown] = "sandyBrown",
          [KnownColours_goldenrod] = "goldenrod",
          [KnownColours_darkGoldenrod] = "darkGoldenrod",
          [KnownColours_peru] = "peru",
          [KnownColours_chocolate] = "chocolate",
          [KnownColours_saddleBrown] = "saddleBrown",
          [KnownColours_sienna] = "sienna",
          [KnownColours_brown] = "brown",
          [KnownColours_maroon] = "maroon",
          [KnownColours_white] = "white",
          [KnownColours_snow] = "snow",
          [KnownColours_honeydew] = "honeydew",
          [KnownColours_mintCream] = "mintCream",
          [KnownColours_azure] = "azure",
          [KnownColours_aliceBlue] = "aliceBlue",
          [KnownColours_ghostWhite] = "ghostWhite",
          [KnownColours_whiteSmoke] = "whiteSmoke",
          [KnownColours_seashell] = "seashell",
          [KnownColours_beige] = "beige",
          [KnownColours_oldLace] = "oldLace",
          [KnownColours_floralWhite] = "floralWhite",
          [KnownColours_ivory] = "ivory",
          [KnownColours_antiqueWhite] = "antiqueWhite",
          [KnownColours_linen] = "linen",
          [KnownColours_lavenderBlush] = "lavenderBlush",
          [KnownColours_mistyRose] = "mistyRose",
          [KnownColours_gainsboro] = "gainsboro",
          [KnownColours_lightGray] = "lightGray",
          [KnownColours_silver] = "silver",
          [KnownColours_darkGray] = "darkGray",
          [KnownColours_gray] = "gray",
          [KnownColours_dimGray] = "dimGray",
          [KnownColours_lightSlateGray] = "lightSlateGray",
          [KnownColours_slateGray] = "slateGray",
          [KnownColours_darkSlateGray] = "darkSlateGray",
          [KnownColours_black] = "black" };

static const Colour KnownColours__values[]
    = { [KnownColours_indianRed] = (Colour) { .value = 0xCD5C5C },
          [KnownColours_lightCoral] = (Colour) { .value = 0xF08080 },
          [KnownColours_salmon] = (Colour) { .value = 0xFA8072 },
          [KnownColours_darkSalmon] = (Colour) { .value = 0xE9967A },
          [KnownColours_lightSalmon] = (Colour) { .value = 0xFFA07A },
          [KnownColours_crimson] = (Colour) { .value = 0xDC143C },
          [KnownColours_red] = (Colour) { .value = 0xFF0000 },
          [KnownColours_fireBrick] = (Colour) { .value = 0xB22222 },
          [KnownColours_darkRed] = (Colour) { .value = 0x8B0000 },
          [KnownColours_pink] = (Colour) { .value = 0xFFC0CB },
          [KnownColours_lightPink] = (Colour) { .value = 0xFFB6C1 },
          [KnownColours_hotPink] = (Colour) { .value = 0xFF69B4 },
          [KnownColours_deepPink] = (Colour) { .value = 0xFF1493 },
          [KnownColours_mediumVioletRed] = (Colour) { .value = 0xC71585 },
          [KnownColours_paleVioletRed] = (Colour) { .value = 0xDB7093 },
          [KnownColours_coral] = (Colour) { .value = 0xFF7F50 },
          [KnownColours_tomato] = (Colour) { .value = 0xFF6347 },
          [KnownColours_orangeRed] = (Colour) { .value = 0xFF4500 },
          [KnownColours_darkOrange] = (Colour) { .value = 0xFF8C00 },
          [KnownColours_orange] = (Colour) { .value = 0xFFA500 },
          [KnownColours_gold] = (Colour) { .value = 0xFFD700 },
          [KnownColours_yellow] = (Colour) { .value = 0xFFFF00 },
          [KnownColours_lightYellow] = (Colour) { .value = 0xFFFFE0 },
          [KnownColours_lemonChiffon] = (Colour) { .value = 0xFFFACD },
          [KnownColours_lightGoldenrodYellow] = (Colour) { .value = 0xFAFAD2 },
          [KnownColours_papayaWhip] = (Colour) { .value = 0xFFEFD5 },
          [KnownColours_moccasin] = (Colour) { .value = 0xFFE4B5 },
          [KnownColours_peachPuff] = (Colour) { .value = 0xFFDAB9 },
          [KnownColours_paleGoldenrod] = (Colour) { .value = 0xEEE8AA },
          [KnownColours_khaki] = (Colour) { .value = 0xF0E68C },
          [KnownColours_darkKhaki] = (Colour) { .value = 0xBDB76B },
          [KnownColours_lavender] = (Colour) { .value = 0xE6E6FA },
          [KnownColours_thistle] = (Colour) { .value = 0xD8BFD8 },
          [KnownColours_plum] = (Colour) { .value = 0xDDA0DD },
          [KnownColours_violet] = (Colour) { .value = 0xEE82EE },
          [KnownColours_orchid] = (Colour) { .value = 0xDA70D6 },
          [KnownColours_fuchsia] = (Colour) { .value = 0xFF00FF },
          [KnownColours_magenta] = (Colour) { .value = 0xFF00FF },
          [KnownColours_mediumOrchid] = (Colour) { .value = 0xBA55D3 },
          [KnownColours_mediumPurple] = (Colour) { .value = 0x9370DB },
          [KnownColours_blueViolet] = (Colour) { .value = 0x8A2BE2 },
          [KnownColours_darkViolet] = (Colour) { .value = 0x9400D3 },
          [KnownColours_darkOrchid] = (Colour) { .value = 0x9932CC },
          [KnownColours_darkMagenta] = (Colour) { .value = 0x8B008B },
          [KnownColours_purple] = (Colour) { .value = 0x800080 },
          [KnownColours_rebeccaPurple] = (Colour) { .value = 0x663399 },
          [KnownColours_indigo] = (Colour) { .value = 0x4B0082 },
          [KnownColours_mediumSlateBlue] = (Colour) { .value = 0x7B68EE },
          [KnownColours_slateBlue] = (Colour) { .value = 0x6A5ACD },
          [KnownColours_darkSlateBlue] = (Colour) { .value = 0x483D8B },
          [KnownColours_greenYellow] = (Colour) { .value = 0xADFF2F },
          [KnownColours_chartreuse] = (Colour) { .value = 0x7FFF00 },
          [KnownColours_lawnGreen] = (Colour) { .value = 0x7CFC00 },
          [KnownColours_lime] = (Colour) { .value = 0x00FF00 },
          [KnownColours_limeGreen] = (Colour) { .value = 0x32CD32 },
          [KnownColours_paleGreen] = (Colour) { .value = 0x98FB98 },
          [KnownColours_lightGreen] = (Colour) { .value = 0x90EE90 },
          [KnownColours_mediumSpringGreen] = (Colour) { .value = 0x00FA9A },
          [KnownColours_springGreen] = (Colour) { .value = 0x00FF7F },
          [KnownColours_mediumSeaGreen] = (Colour) { .value = 0x3CB371 },
          [KnownColours_seaGreen] = (Colour) { .value = 0x2E8B57 },
          [KnownColours_forestGreen] = (Colour) { .value = 0x228B22 },
          [KnownColours_green] = (Colour) { .value = 0x008000 },
          [KnownColours_darkGreen] = (Colour) { .value = 0x006400 },
          [KnownColours_yellowGreen] = (Colour) { .value = 0x9ACD32 },
          [KnownColours_oliveDrab] = (Colour) { .value = 0x6B8E23 },
          [KnownColours_olive] = (Colour) { .value = 0x808000 },
          [KnownColours_darkOliveGreen] = (Colour) { .value = 0x556B2F },
          [KnownColours_mediumAquamarine] = (Colour) { .value = 0x66CDAA },
          [KnownColours_darkSeaGreen] = (Colour) { .value = 0x8FBC8F },
          [KnownColours_lightSeaGreen] = (Colour) { .value = 0x20B2AA },
          [KnownColours_darkCyan] = (Colour) { .value = 0x008B8B },
          [KnownColours_teal] = (Colour) { .value = 0x008080 },
          [KnownColours_aqua] = (Colour) { .value = 0x00FFFF },
          [KnownColours_cyan] = (Colour) { .value = 0x00FFFF },
          [KnownColours_lightCyan] = (Colour) { .value = 0xE0FFFF },
          [KnownColours_paleTurquoise] = (Colour) { .value = 0xAFEEEE },
          [KnownColours_aquamarine] = (Colour) { .value = 0x7FFFD4 },
          [KnownColours_turquoise] = (Colour) { .value = 0x40E0D0 },
          [KnownColours_mediumTurquoise] = (Colour) { .value = 0x48D1CC },
          [KnownColours_darkTurquoise] = (Colour) { .value = 0x00CED1 },
          [KnownColours_cadetBlue] = (Colour) { .value = 0x5F9EA0 },
          [KnownColours_steelBlue] = (Colour) { .value = 0x4682B4 },
          [KnownColours_lightSteelBlue] = (Colour) { .value = 0xB0C4DE },
          [KnownColours_powderBlue] = (Colour) { .value = 0xB0E0E6 },
          [KnownColours_lightBlue] = (Colour) { .value = 0xADD8E6 },
          [KnownColours_skyBlue] = (Colour) { .value = 0x87CEEB },
          [KnownColours_lightSkyBlue] = (Colour) { .value = 0x87CEFA },
          [KnownColours_deepSkyBlue] = (Colour) { .value = 0x00BFFF },
          [KnownColours_dodgerBlue] = (Colour) { .value = 0x1E90FF },
          [KnownColours_cornflowerBlue] = (Colour) { .value = 0x6495ED },
          [KnownColours_royalBlue] = (Colour) { .value = 0x4169E1 },
          [KnownColours_blue] = (Colour) { .value = 0x0000FF },
          [KnownColours_mediumBlue] = (Colour) { .value = 0x0000CD },
          [KnownColours_darkBlue] = (Colour) { .value = 0x00008B },
          [KnownColours_navy] = (Colour) { .value = 0x000080 },
          [KnownColours_midnightBlue] = (Colour) { .value = 0x191970 },
          [KnownColours_cornsilk] = (Colour) { .value = 0xFFF8DC },
          [KnownColours_blanchedAlmond] = (Colour) { .value = 0xFFEBCD },
          [KnownColours_bisque] = (Colour) { .value = 0xFFE4C4 },
          [KnownColours_navajoWhite] = (Colour) { .value = 0xFFDEAD },
          [KnownColours_wheat] = (Colour) { .value = 0xF5DEB3 },
          [KnownColours_burlyWood] = (Colour) { .value = 0xDEB887 },
          [KnownColours_tan] = (Colour) { .value = 0xD2B48C },
          [KnownColours_rosyBrown] = (Colour) { .value = 0xBC8F8F },
          [KnownColours_sandyBrown] = (Colour) { .value = 0xF4A460 },
          [KnownColours_goldenrod] = (Colour) { .value = 0xDAA520 },
          [KnownColours_darkGoldenrod] = (Colour) { .value = 0xB8860B },
          [KnownColours_peru] = (Colour) { .value = 0xCD853F },
          [KnownColours_chocolate] = (Colour) { .value = 0xD2691E },
          [KnownColours_saddleBrown] = (Colour) { .value = 0x8B4513 },
          [KnownColours_sienna] = (Colour) { .value = 0xA0522D },
          [KnownColours_brown] = (Colour) { .value = 0xA52A2A },
          [KnownColours_maroon] = (Colour) { .value = 0x800000 },
          [KnownColours_white] = (Colour) { .value = 0xFFFFFF },
          [KnownColours_snow] = (Colour) { .value = 0xFFFAFA },
          [KnownColours_honeydew] = (Colour) { .value = 0xF0FFF0 },
          [KnownColours_mintCream] = (Colour) { .value = 0xF5FFFA },
          [KnownColours_azure] = (Colour) { .value = 0xF0FFFF },
          [KnownColours_aliceBlue] = (Colour) { .value = 0xF0F8FF },
          [KnownColours_ghostWhite] = (Colour) { .value = 0xF8F8FF },
          [KnownColours_whiteSmoke] = (Colour) { .value = 0xF5F5F5 },
          [KnownColours_seashell] = (Colour) { .value = 0xFFF5EE },
          [KnownColours_beige] = (Colour) { .value = 0xF5F5DC },
          [KnownColours_oldLace] = (Colour) { .value = 0xFDF5E6 },
          [KnownColours_floralWhite] = (Colour) { .value = 0xFFFAF0 },
          [KnownColours_ivory] = (Colour) { .value = 0xFFFFF0 },
          [KnownColours_antiqueWhite] = (Colour) { .value = 0xFAEBD7 },
          [KnownColours_linen] = (Colour) { .value = 0xFAF0E6 },
          [KnownColours_lavenderBlush] = (Colour) { .value = 0xFFF0F5 },
          [KnownColours_mistyRose] = (Colour) { .value = 0xFFE4E1 },
          [KnownColours_gainsboro] = (Colour) { .value = 0xDCDCDC },
          [KnownColours_lightGray] = (Colour) { .value = 0xD3D3D3 },
          [KnownColours_silver] = (Colour) { .value = 0xC0C0C0 },
          [KnownColours_darkGray] = (Colour) { .value = 0xA9A9A9 },
          [KnownColours_gray] = (Colour) { .value = 0x808080 },
          [KnownColours_dimGray] = (Colour) { .value = 0x696969 },
          [KnownColours_lightSlateGray] = (Colour) { .value = 0x778899 },
          [KnownColours_slateGray] = (Colour) { .value = 0x708090 },
          [KnownColours_darkSlateGray] = (Colour) { .value = 0x2F4F4F },
          [KnownColours_black] = (Colour) { .value = 0x000000 } };

static const char* const KnownColours__groupNames[]
    = { [KnownColours_indianRed] = "Reds",
          [KnownColours_lightCoral] = "Reds",
          [KnownColours_salmon] = "Reds",
          [KnownColours_darkSalmon] = "Reds",
          [KnownColours_lightSalmon] = "Reds",
          [KnownColours_crimson] = "Reds",
          [KnownColours_red] = "Reds",
          [KnownColours_fireBrick] = "Reds",
          [KnownColours_darkRed] = "Reds",
          [KnownColours_pink] = "Pinks",
          [KnownColours_lightPink] = "Pinks",
          [KnownColours_hotPink] = "Pinks",
          [KnownColours_deepPink] = "Pinks",
          [KnownColours_mediumVioletRed] = "Pinks",
          [KnownColours_paleVioletRed] = "Pinks",
          [KnownColours_coral] = "Oranges",
          [KnownColours_tomato] = "Oranges",
          [KnownColours_orangeRed] = "Oranges",
          [KnownColours_darkOrange] = "Oranges",
          [KnownColours_orange] = "Oranges",
          [KnownColours_gold] = "Yellows",
          [KnownColours_yellow] = "Yellows",
          [KnownColours_lightYellow] = "Yellows",
          [KnownColours_lemonChiffon] = "Yellows",
          [KnownColours_lightGoldenrodYellow] = "Yellows",
          [KnownColours_papayaWhip] = "Yellows",
          [KnownColours_moccasin] = "Yellows",
          [KnownColours_peachPuff] = "Yellows",
          [KnownColours_paleGoldenrod] = "Yellows",
          [KnownColours_khaki] = "Yellows",
          [KnownColours_darkKhaki] = "Yellows",
          [KnownColours_lavender] = "Purples",
          [KnownColours_thistle] = "Purples",
          [KnownColours_plum] = "Purples",
          [KnownColours_violet] = "Purples",
          [KnownColours_orchid] = "Purples",
          [KnownColours_fuchsia] = "Purples",
          [KnownColours_magenta] = "Purples",
          [KnownColours_mediumOrchid] = "Purples",
          [KnownColours_mediumPurple] = "Purples",
          [KnownColours_blueViolet] = "Purples",
          [KnownColours_darkViolet] = "Purples",
          [KnownColours_darkOrchid] = "Purples",
          [KnownColours_darkMagenta] = "Purples",
          [KnownColours_purple] = "Purples",
          [KnownColours_rebeccaPurple] = "Purples",
          [KnownColours_indigo] = "Purples",
          [KnownColours_mediumSlateBlue] = "Purples",
          [KnownColours_slateBlue] = "Purples",
          [KnownColours_darkSlateBlue] = "Purples",
          [KnownColours_greenYellow] = "Greens",
          [KnownColours_chartreuse] = "Greens",
          [KnownColours_lawnGreen] = "Greens",
          [KnownColours_lime] = "Greens",
          [KnownColours_limeGreen] = "Greens",
          [KnownColours_paleGreen] = "Greens",
          [KnownColours_lightGreen] = "Greens",
          [KnownColours_mediumSpringGreen] = "Greens",
          [KnownColours_springGreen] = "Greens",
          [KnownColours_mediumSeaGreen] = "Greens",
          [KnownColours_seaGreen] = "Greens",
          [KnownColours_forestGreen] = "Greens",
          [KnownColours_green] = "Greens",
          [KnownColours_darkGreen] = "Greens",
          [KnownColours_yellowGreen] = "Greens",
          [KnownColours_oliveDrab] = "Greens",
          [KnownColours_olive] = "Greens",
          [KnownColours_darkOliveGreen] = "Greens",
          [KnownColours_mediumAquamarine] = "Greens",
          [KnownColours_darkSeaGreen] = "Greens",
          [KnownColours_lightSeaGreen] = "Greens",
          [KnownColours_darkCyan] = "Greens",
          [KnownColours_teal] = "Greens",
          [KnownColours_aqua] = "Blues",
          [KnownColours_cyan] = "Blues",
          [KnownColours_lightCyan] = "Blues",
          [KnownColours_paleTurquoise] = "Blues",
          [KnownColours_aquamarine] = "Blues",
          [KnownColours_turquoise] = "Blues",
          [KnownColours_mediumTurquoise] = "Blues",
          [KnownColours_darkTurquoise] = "Blues",
          [KnownColours_cadetBlue] = "Blues",
          [KnownColours_steelBlue] = "Blues",
          [KnownColours_lightSteelBlue] = "Blues",
          [KnownColours_powderBlue] = "Blues",
          [KnownColours_lightBlue] = "Blues",
          [KnownColours_skyBlue] = "Blues",
          [KnownColours_lightSkyBlue] = "Blues",
          [KnownColours_deepSkyBlue] = "Blues",
          [KnownColours_dodgerBlue] = "Blues",
          [KnownColours_cornflowerBlue] = "Blues",
          [KnownColours_royalBlue] = "Blues",
          [KnownColours_blue] = "Blues",
          [KnownColours_mediumBlue] = "Blues",
          [KnownColours_darkBlue] = "Blues",
          [KnownColours_navy] = "Blues",
          [KnownColours_midnightBlue] = "Blues",
          [KnownColours_cornsilk] = "Browns",
          [KnownColours_blanchedAlmond] = "Browns",
          [KnownColours_bisque] = "Browns",
          [KnownColours_navajoWhite] = "Browns",
          [KnownColours_wheat] = "Browns",
          [KnownColours_burlyWood] = "Browns",
          [KnownColours_tan] = "Browns",
          [KnownColours_rosyBrown] = "Browns",
          [KnownColours_sandyBrown] = "Browns",
          [KnownColours_goldenrod] = "Browns",
          [KnownColours_darkGoldenrod] = "Browns",
          [KnownColours_peru] = "Browns",
          [KnownColours_chocolate] = "Browns",
          [KnownColours_saddleBrown] = "Browns",
          [KnownColours_sienna] = "Browns",
          [KnownColours_brown] = "Browns",
          [KnownColours_maroon] = "Browns",
          [KnownColours_white] = "Whites",
          [KnownColours_snow] = "Whites",
          [KnownColours_honeydew] = "Whites",
          [KnownColours_mintCream] = "Whites",
          [KnownColours_azure] = "Whites",
          [KnownColours_aliceBlue] = "Whites",
          [KnownColours_ghostWhite] = "Whites",
          [KnownColours_whiteSmoke] = "Whites",
          [KnownColours_seashell] = "Whites",
          [KnownColours_beige] = "Whites",
          [KnownColours_oldLace] = "Whites",
          [KnownColours_floralWhite] = "Whites",
          [KnownColours_ivory] = "Whites",
          [KnownColours_antiqueWhite] = "Whites",
          [KnownColours_linen] = "Whites",
          [KnownColours_lavenderBlush] = "Whites",
          [KnownColours_mistyRose] = "Whites",
          [KnownColours_gainsboro] = "Greys",
          [KnownColours_lightGray] = "Greys",
          [KnownColours_silver] = "Greys",
          [KnownColours_darkGray] = "Greys",
          [KnownColours_gray] = "Greys",
          [KnownColours_dimGray] = "Greys",
          [KnownColours_lightSlateGray] = "Greys",
          [KnownColours_slateGray] = "Greys",
          [KnownColours_darkSlateGray] = "Greys",
          [KnownColours_black] = "Greys" };

#define jet_countof(x) (sizeof(x) / sizeof(x[0]))
int power2(int a) { return a * a; }

#include <math.h>

double ColourDistance(Colour e1, Colour e2)
{
    long rmean = ((long)e1.red + (long)e2.red) / 2;
    long r = ((long)e1.red - (long)e2.red);
    long g = ((long)e1.green - (long)e2.green);
    long b = ((long)e1.blue - (long)e2.blue);
    return sqrt((((512 + rmean) * r * r) >> 8) + 4 * g * g
        + (((767 - rmean) * b * b) >> 8));
}

typedef struct {
    int L, a, b;
} LabColour;

LabColour rgb2lab(Colour col)
{
    // http://www.brucelindbloom.com

    float r, g, b, X, Y, Z, fx, fy, fz, xr, yr, zr;
    float Ls, as, bs;
    float eps = 216.f / 24389.f;
    float k = 24389.f / 27.f;

    float Xr = 0.964221f; // reference white D50
    float Yr = 1.0f;
    float Zr = 0.825211f;

    // RGB to XYZ
    r = col.red / 255.f; // R 0..1
    g = col.green / 255.f; // G 0..1
    b = col.blue / 255.f; // B 0..1

    // assuming sRGB (D65)
    if (r <= 0.04045)
        r = r / 12;
    else
        r = (float)pow((r + 0.055) / 1.055, 2.4);

    if (g <= 0.04045)
        g = g / 12;
    else
        g = (float)pow((g + 0.055) / 1.055, 2.4);

    if (b <= 0.04045)
        b = b / 12;
    else
        b = (float)pow((b + 0.055) / 1.055, 2.4);

    X = 0.436052025f * r + 0.385081593f * g + 0.143087414f * b;
    Y = 0.222491598f * r + 0.71688606f * g + 0.060621486f * b;
    Z = 0.013929122f * r + 0.097097002f * g + 0.71418547f * b;

    // XYZ to Lab
    xr = X / Xr;
    yr = Y / Yr;
    zr = Z / Zr;

    if (xr > eps)
        fx = (float)pow(xr, 1 / 3.);
    else
        fx = (float)((k * xr + 16.) / 116.);

    if (yr > eps)
        fy = (float)pow(yr, 1 / 3.);
    else
        fy = (float)((k * yr + 16.) / 116.);

    if (zr > eps)
        fz = (float)pow(zr, 1 / 3.);
    else
        fz = (float)((k * zr + 16.) / 116);

    Ls = (116 * fy) - 16;
    as = 500 * (fx - fy);
    bs = 200 * (fy - fz);

    return (LabColour) {
        .L = (int)(2.55 * Ls + .5), .a = (int)(as + .5), .b = (int)(bs + .5)
    };
}

/**
 * Computes the difference between two RGB colors by converting them to the
 * L*a*b scale and comparing them using the CIE76 algorithm {
 * http://en.wikipedia.org/wiki/Color_difference#CIE76}
 */
double getLabColorDifferenceSq(Colour a, Colour b)
{
    //    int r1, g1, b1, r2, g2, b2;
    //    r1 = a.red;
    //    g1 = a.green;
    //    b1 = a.blue;
    //    r2 = b.red;
    //    g2 = b.green;
    //    b2 = b.blue;
    LabColour lab1 = rgb2lab(a);
    LabColour lab2 = rgb2lab(b);
    return pow(lab2.L - lab1.L, 2) + pow(lab2.a - lab1.a, 2)
        + pow(lab2.b - lab1.b, 2);
}

enum KnownColours closestKnown(Colour col)
{
    //    unsigned long long dsq = UINT64_MAX;
    double dsq = 1e100;
    int ret = -1;
    for (int i = 0; i < jet_countof(KnownColours__names); i++) {
        Colour kc = KnownColours__values[i];
        //        unsigned long long dsqi = 0;
        // may need to promote to wider int before subtracting
        //        double dsqi=ColourDistance(kc, col);
        double dsqi = getLabColorDifferenceSq(kc, col);
        int kcred = kc.red;
        int kcgreen = kc.green;
        int kcblue = kc.blue;
        int colred = col.red;
        int colgreen = col.green;
        int colblue = col.blue;
        //
        //        dsqi += power2(1+colred)*power2(kcred-colred);
        //        dsqi +=  power2(1+colgreen)*power2(kcgreen-colgreen);
        //        dsqi +=  power2(1+colblue)*power2(kcblue-colblue);
        if (dsqi < dsq) {
            dsq = dsqi;
            ret = i;
            //            printf("%llu %llu : %d %d %d - %d %d %d - %s\n",dsq,
            //            dsqi,kcred,kcgreen,kcblue,colred,colgreen,colblue,KnownColours__names[i]);
        }
    }
    return ret;
}

int maisn()
{
    unsigned int kcols[] = { 0x08653e, 0xfedcba, 0x128E9A, 0x586264 };
    for (int i = 0; i < jet_countof(kcols); i++) {
        puts("----");
        Colour c = Colour_new(kcols[i]);
        rgb2lab(c);
        Colour_print(c);
        Colour_printRGBA(c);
        enum KnownColours kc = closestKnown(c);
        printf("closest: %s (%s) ", KnownColours__names[kc],
            KnownColours__groupNames[kc]);
        Colour_print(KnownColours__values[kc]);
    }
    return 0;
}

#include "regex.h"

#define NREGEX_MAX_SUBMATCH 5
typedef struct {
    regex_t prog;
} _RegexProg;

typedef struct {
    regmatch_t sub[NREGEX_MAX_SUBMATCH + 1]; // 96B. You can have 5 submatches.
} RegexMatch;

// _RegexProg is not really exposed in F+. why keep it public? Just hide it
// since regex literals and strings passed in to regex args will be
// transparently wrapped into a RegexProg_new call. take care to "optimise"
// literals, replace char classes e.g. \d \D etc.
_RegexProg Regex__compile(const char* str, int matchCase, int justMatch)
{
    regex_t reg;
    matchCase = (!matchCase) * REG_ICASE;
    justMatch = (!!justMatch) * REG_NOSUB;
    int err;
    if (err = regcomp(&reg, str, REG_EXTENDED | matchCase | justMatch))
        ; // handle error
    return (_RegexProg) { .prog = reg };
}
static const long szs = sizeof(regex_t);
static const long sza = sizeof(RegexMatch);
static const long szv = sizeof(_RegexProg);
static const long sze = sizeof(regmatch_t);

RegexMatch Regex_match(_RegexProg prog, char* source)
{
    RegexMatch match = {};
    int err;
    if (err = regexec(&prog.prog, source, NREGEX_MAX_SUBMATCH, match.sub, 0))
        ; // handle error
    return match;
}

int Regex_contains(_RegexProg prog, char* source) // yes or no
{
    RegexMatch match = {};
    int err;
    if (err = regexec(&prog.prog, source, NREGEX_MAX_SUBMATCH, match.sub, 0))
        ; // handle error
    return !err;
}

/* substitute into one string using the matches from the last regexec() */
// this needs work, it just works locally on a new buffer, not the original
// source string
static size_t _regsub(
    char* replacement, char* buf, int dlen, regmatch_t* match, int nmatch)
{
    char* origSource = replacement;
    char* origDest = buf;
    char *start, *end;
    int i;

    end = buf + dlen - 1;
    while (*replacement != '\0') {
        if (*replacement == '\\') {
            switch (*++replacement) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                i = *replacement - '0';
                if (match != NULL && nmatch > i)
                    for (start = origSource + match[i].rm_so;
                         start < origSource + match[i].rm_eo; start++)
                        if (buf < end) *buf++ = *start;
                break;
            case '\\':
                if (buf < end) *buf++ = '\\';
                break;
            case '\0':
                replacement--;
                break;
            default:
                if (buf < end) *buf++ = *replacement;
                break;
            }
        } else if (*replacement == '&') {
            if (match != NULL && nmatch > 0)
                for (start = origSource + match[0].rm_so;
                     start < origSource + match[0].rm_eo; start++)
                    if (buf < end) *buf++ = *start;
        } else {
            if (buf < end) *buf++ = *replacement;
        }
        replacement++;
    }
    *buf = '\0';

    return buf - origDest;
}

#define Text char*
// be careful regsub does not allocate or check buffer size
Text Regex_replace(RegexMatch match, char* source, char* replacement)
{
    Text str = malloc(1 /* FIXME */);
    size_t written = _regsub(source, str, 1, match.sub, NREGEX_MAX_SUBMATCH);

    return "";
}

int maisan() { return 0; }

#include <complex.h>
typedef double complex Complex;
Complex Complex_new(double re, double im) { return CMPLX(re, im); }
Complex Complex_acos(Complex c) { return cacos(c); };
Complex Complex_asin(Complex c) { return casin(c); };
Complex Complex_atan(Complex c) { return catan(c); };
Complex Complex_cos(Complex c) { return ccos(c); };
Complex Complex_sin(Complex c) { return csin(c); };
Complex Complex_tan(Complex c) { return ctan(c); };
Complex Complex_acosh(Complex c) { return cacosh(c); };
Complex Complex_asinh(Complex c) { return casinh(c); };
Complex Complex_atanh(Complex c) { return catanh(c); };
Complex Complex_cosh(Complex c) { return ccosh(c); };
Complex Complex_sinh(Complex c) { return csinh(c); };
Complex Complex_tanh(Complex c) { return ctanh(c); };
Complex Complex_exp(Complex c) { return cexp(c); };
Complex Complex_log(Complex c) { return clog(c); };
Complex Complex_pow(Complex c, Complex p) { return cpow(c, p); };
Complex Complex_sqrt(Complex c) { return csqrt(c); };
Complex Complex_conj(Complex c) { return conj(c); };
Complex Complex_proj(Complex c) { return cproj(c); };
Real Complex_abs(Complex c) { return cabs(c); };
Real Complex_arg(Complex c) { return carg(c); };
Real Complex_imag(Complex c) { return cimag(c); };
Real Complex_real(Complex c) { return creal(c); };

typedef float complex Complex4;
static inline Complex4 Complex4_new(double re, double im)
{
    return CMPLX(re, im);
}
static inline Complex4 Complex4_acos(Complex4 c) { return cacosf(c); };
static inline Complex4 Complex4_asin(Complex4 c) { return casinf(c); };
static inline Complex4 Complex4_atan(Complex4 c) { return catanf(c); };
static inline Complex4 Complex4_cos(Complex4 c) { return ccosf(c); };
static inline Complex4 Complex4_sin(Complex4 c) { return csinf(c); };
static inline Complex4 Complex4_tan(Complex4 c) { return ctanf(c); };
static inline Complex4 Complex4_acosh(Complex4 c) { return cacoshf(c); };
static inline Complex4 Complex4_asinh(Complex4 c) { return casinhf(c); };
static inline Complex4 Complex4_atanh(Complex4 c) { return catanhf(c); };
static inline Complex4 Complex4_cosh(Complex4 c) { return ccoshf(c); };
static inline Complex4 Complex4_sinh(Complex4 c) { return csinhf(c); };
static inline Complex4 Complex4_tanh(Complex4 c) { return ctanhf(c); };
static inline Complex4 Complex4_exp(Complex4 c) { return cexpf(c); };
static inline Complex4 Complex4_log(Complex4 c) { return clogf(c); };
static inline Complex4 Complex4_pow(Complex4 c, Complex4 p)
{
    return cpowf(c, p);
};
static inline Complex4 Complex4_sqrt(Complex4 c) { return csqrtf(c); };
static inline Complex4 Complex4_conj(Complex4 c) { return conjf(c); };
static inline Complex4 Complex4_proj(Complex4 c) { return cprojf(c); };
static inline Real32 Complex4_abs(Complex4 c) { return cabsf(c); };
static inline Real32 Complex4_arg(Complex4 c) { return cargf(c); };
static inline Real32 Complex4_imag(Complex4 c) { return cimagf(c); };
static inline Real32 Complex4_real(Complex4 c) { return crealf(c); };

int psz()
{
    printf("int: %lu\n", sizeof(int));
    printf("char: %lu\n", sizeof(char));
    printf("long: %lu\n", sizeof(long));
    printf("double: %lu\n", sizeof(double));
    printf("float: %lu\n", sizeof(float));
    printf("unsigned int: %lu\n", sizeof(unsigned int));
    printf("unsigned: %lu\n", sizeof(unsigned));
    printf("unsigned long: %lu\n", sizeof(unsigned long));
    printf("long long: %lu\n", sizeof(long long));
    printf("long long int: %lu\n", sizeof(long long int));
    printf("short: %lu\n", sizeof(short));
    return 0;
}

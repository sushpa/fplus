#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "assert.h"
#include "math.h"
#include "limits.h"
#include <float.h>

#include "fp_base.h"
MKSTAT(Intv)
MKSTAT(FIntv)

int64_t signed_saturated_add(int64_t x, int64_t y, int* didOverflow)
{
    // determine the lower or upper bound of the result
    int64_t ret = (x < 0) ? INT64_MIN : INT64_MAX;
    // this is always well defined:
    // if x < 0 this adds a positive value to INT64_MIN
    // if x > 0 this subtracts a positive value from INT64_MAX
    int64_t comp = ret - x;
    // the condition is equivalent to
    // ((x < 0) && (y > comp)) || ((x >=0) && (y <= comp))
    if ((x < 0) == (y > comp))
        ret = x + y;
    else
        *didOverflow = 1;
    return ret;
}
int64_t signed_saturated_sub(int64_t x, int64_t y, int* didOverflow)
{
    // CAREFUL: negating INT64_MAX overflows
    return signed_saturated_add(x, -y, didOverflow);
}

int64_t signed_saturated_mul(int64_t x, int64_t y, int* didOverflow)
{
    // determine the lower or upper bound of the result
    bool oppsgn = x < 0 != y < 0;
    int64_t ret = (oppsgn) ? INT64_MIN + 1 : INT64_MAX;
    // printf("x = %lld\n", x);
    // printf("y = %lld\n", y);
    // printf("ret = %lld\n", ret);
    int64_t comp = llabs(x ? ret / x : ret);
    // printf("comp = %lld\n", comp);

    // if ((x < 0) == (y > comp))
    if ((y < comp))
        ret = x * y;
    else
        *didOverflow = 1;
    return ret;
}

// static bool canoverflow_add(int64_t a, int64_t x)
// {
//     return ((x > 0) && (a > INT64_MAX - x))
//         /* `a + x` would overflow */
//         or ((x < 0) && (a < INT64_MIN - x))
//         /* `a + x` would underflow */;
// }

// static bool canoverflow_sub(int64_t a, int64_t x)
// {
//     return ((x < 0) && (a > INT64_MAX + x))
//         /* `a - x` would overflow */
//         or ((x > 0) && (a < INT64_MIN + x))
//         /* `a - x` would underflow */;
// }

// static bool canoverflow_mul(int64_t a, int64_t x)
// { // There may be a need to check for -1 for two's complement machines.
//     // If one number is -1 and another is INT_MIN, multiplying them we get
//     // abs(INT_MIN) which is 1 higher than INT_MAX
//     return x
//         and ((a > INT64_MAX / x)
//             /* `a * x` would overflow */
//             or ((a < INT64_MIN / x))
//             /* `a * x` would underflow */
//             or ((a == -1) && (x == INT64_MIN))
//             /* `a * x` can overflow */
//             or ((x == -1) && (a == INT64_MIN)));
//     /* `a * x` (or `a / x`) can overflow */
// }

// API:
// I_del: delete
// I_ins: insert
// I_add: apply a + on the entire interval
// I_sub: apply -
// I_mul: apply *
// I_div: apply /
// I_pow: apply ^
// I_mod: apply %
// I_rcp: reciprocal
// I_neg: negate
// I_has: check value in range

typedef struct Intv {
    int64_t lo, hi;
    struct Intv* next;
} Intv;

typedef struct FIntv {
    double lo, hi;
    struct FIntv* next;
} FIntv;
// typedef PtrIist Intv;

static int64_t max1(int64_t a, int64_t b) { return a > b ? a : b; }

static int64_t min1(int64_t a, int64_t b) { return a < b ? a : b; }

static double fmax1(double a, double b) { return a > b ? a : b; }

static double fmin1(double a, double b) { return a < b ? a : b; }

// static Intv Interval_add(Intv intv, Intv other)
// {
//     return (Intv) { .lo = intv.lo + other.lo, .hi = intv.hi + other.hi };
// }

#define I_desc(a)                                                              \
    {                                                                          \
        printf("%s:%d: %s = ", __FILE__, __LINE__, #a);                        \
        I_print(a);                                                            \
    }
#define F_desc(a)                                                              \
    {                                                                          \
        printf("%s:%d: %s = ", __FILE__, __LINE__, #a);                        \
        F_print(a);                                                            \
    }
static void I_print1(Intv* intv) { printf("%lld:%lld\n", intv->lo, intv->hi); }
static void F_print1(FIntv* fintv) { printf("%g:%g\n", fintv->lo, fintv->hi); }

static void I_print(Intv* intv)
{
    putc('[', stdout);
    while (intv) {
        printf("%lld:%lld", intv->lo, intv->hi);
        intv = intv->next;
        if (intv) printf(", ");
    }
    puts("]");
}
static void F_print(FIntv* fintv)
{
    putc('[', stdout);
    while (fintv) {
        printf("%g:%g", fintv->lo, fintv->hi);
        fintv = fintv->next;
        if (fintv) printf(", ");
    }
    puts("]");
}
static void I_jins(Intv* intv, Intv* other);
static void F_jins(FIntv* fintv, FIntv* other);
static void I_rev(Intv* intv)
{
    // printf("reversing ");
    // I_print(intv);
    Intv* orig = intv;
    while (intv and intv->next) intv = intv->next;
    Intv tmp = *intv;
    intv = orig;
    while (intv) {
        // Intv* tmp = intv->next->next;
        // intv->next->next = intv;
        // intv->next = tmp;
        I_jins(&tmp, intv);
        intv = intv->next;
    }
    *orig = tmp;
}
static void F_rev(FIntv* fintv)
{
    // printf("reversing ");
    // I_print(intv);
    FIntv* orig = fintv;
    while (fintv and fintv->next) fintv = fintv->next;
    FIntv tmp = *fintv;
    fintv = orig;
    while (fintv) {
        // Intv* tmp = intv->next->next;
        // intv->next->next = intv;
        // intv->next = tmp;
        F_jins(&tmp, fintv);
        fintv = fintv->next;
    }
    *orig = tmp;
}
static void I_snap(Intv* intv)
{
    // return;
    // printf("snapping ");
    // I_print(intv);
    while (intv) {
        while (intv->next and intv->lo <= intv->next->lo
            and intv->hi + 1 >= intv->next->lo) {
            intv->hi = max1(intv->hi, intv->next->hi);
            intv->lo = min1(intv->lo, intv->next->lo);
            intv->next = intv->next->next;
        }
        intv = intv->next;
    }
}
static void F_snap(FIntv* fintv)
{
    // return;
    // printf("snapping ");
    // I_print(intv);
    while (fintv) {
        while (fintv->next and fintv->lo <= fintv->next->lo
            and nextafter(fintv->hi, DBL_MAX) >= fintv->next->lo) {
            fintv->hi = fmax1(fintv->hi, fintv->next->hi);
            fintv->lo = fmin1(fintv->lo, fintv->next->lo);
            fintv->next = fintv->next->next;
        }
        fintv = fintv->next;
    }
}
static void I_add(Intv* intv, Intv* other)
{
    Intv* orig = intv;
    while (intv) {
        int* stat = (int[]) { 0 };
        intv->lo = signed_saturated_add(intv->lo, other->lo, stat);
        intv->hi = signed_saturated_add(intv->hi, other->hi, stat);
        // intv->lo += other->lo;
        // intv->hi += other->hi;
        if (*stat) printf("add overflowed\n");
        intv = intv->next;
    }
    I_snap(orig);
    // do need to snap, since the added value can be large enough to cause hi to
    // reach over the next lo
}
static void F_add(FIntv* fintv, FIntv* other)
{
    FIntv* orig = fintv;
    while (fintv) {
        fintv->lo += other->lo;
        fintv->hi += other->hi;

        fintv = fintv->next;
    }
    F_snap(orig);
}

static void I_sub(Intv* intv, Intv* other)
{
    Intv flipped = { .lo = -other->hi, .hi = -other->lo };
    I_add(intv, &flipped);
}

static void F_sub(FIntv* fintv, FIntv* other)
{
    FIntv flipped = { .lo = -other->hi, .hi = -other->lo };
    F_add(fintv, &flipped);
}

static void minmax(int n, int64_t arr[], int64_t* min, int64_t* max)
{
    int64_t mmin = INT64_MAX, mmax = INT64_MIN;
    for (int i = 0; i < n; i++) {
        mmin = min1(mmin, arr[i]);
        mmax = max1(mmax, arr[i]);
    }
    *min = mmin;
    *max = mmax;
}

static void fminmax(int n, double arr[], double* min, double* max)
{
    double mmin = DBL_MAX, mmax = -DBL_MAX; // FIXME
    for (int i = 0; i < n; i++) {
        mmin = fmin1(mmin, arr[i]);
        mmax = fmax1(mmax, arr[i]);
    }
    *min = mmin;
    *max = mmax;
}

static void I_mul(Intv* intv, Intv* other)
{
    Intv* orig = intv;
    int64_t a, b, c, d; // minv, maxv;
    while (intv) {
        int* stat = (int[]) { 0 };
        // TODO: if (stat)... at every step
        a = signed_saturated_mul(intv->lo, other->lo, stat);
        b = signed_saturated_mul(intv->hi, other->lo, stat);
        c = signed_saturated_mul(intv->lo, other->hi, stat);
        //    if (stat) ...
        d = signed_saturated_mul(intv->hi, other->hi, stat);
        if (*stat) printf("mul overflowed\n");
        minmax(4, (int64_t[]) { a, b, c, d }, &(intv->lo), &(intv->hi));

        intv = intv->next;
    }
    if (other->lo <= 0 == other->hi <= 0) {
        // reverse the list
        I_rev(orig);
    }
    I_snap(orig);
    // intv->lo=minv,
}

static void F_mul(FIntv* fintv, FIntv* other)
{
    FIntv* orig = fintv;
    double a, b, c, d;
    // F_print1(fintv);
    // F_print1(other);
    while (fintv) {
        a = fintv->lo * other->lo;
        b = fintv->hi * other->lo;
        c = fintv->lo * other->hi;
        d = fintv->hi * other->hi;
        // printf("-- %g %g %g %g\n", a, b, c, d);
        fminmax(4, (double[]) { a, b, c, d }, &(fintv->lo), &(fintv->hi));
        // F_print1(fintv);
        fintv = fintv->next;
    }

    if (other->lo <= 0 and other->hi <= 0) F_rev(orig);
    F_snap(orig);
}

// #define mapf(intv, other, func)                                                \
//     while (intv) {                                                             \
//         func(intv->item, other);                                               \
//         intv = intv->next;                                                     \
//     }

// static int Interval_contains(  Intv*   intv,   int64_t value)
// {
//     return intv->lo <= value && value <= intv->hi;
// }
// static void I_print(  Intv*   intv)
// {
//     printf("%lld:%lld", intv->lo, intv->hi);
// }

// returns the pointer to the IIST ITEM containing the value.
// if you want the interval, you should use ->item of the returned ptr.
static Intv* I_find(Intv* intv, int64_t value)
{
    // if you can keep the list sorted, you can just check the extremities first
    // guess you must sort, its needed for coalescing if you want to avoid
    // O(N^2)
    while (intv) {
        if (intv->lo > value)
            return NULL;
        else if (value <= intv->hi)
            return intv;
        intv = intv->next;
    }
    return NULL;
}

static FIntv* F_find(FIntv* fintv, double value)
{
    // if you can keep the list sorted, you can just check the extremities first
    // guess you must sort, its needed for coalescing if you want to avoid
    // O(N^2)
    while (fintv) {
        if (fintv->lo > value)
            return NULL;
        else if (value <= fintv->hi)
            return fintv;
        fintv = fintv->next;
    }
    return NULL;
}

static void I_ins1(Intv* intv, int64_t value)
{
    while (intv and intv->next and intv->next->lo < value) intv = intv->next;
    I_print1(intv);

    // intv->next = PtrIist_with_next(other, intv->next);
}

static Intv* I_new(int64_t lo, int64_t hi, Intv* next)
{
    Intv* I = NEW(Intv);
    I->hi = hi;
    I->lo = lo;
    I->next = next;
    return I;
}

static FIntv* F_new(double lo, double hi, FIntv* next)
{
    FIntv* I = NEW(FIntv);
    I->hi = hi;
    I->lo = lo;
    I->next = next;
    return I;
}

static FIntv* F_new1(double value, FIntv* next)
{
    return F_new(value, value, next);
}

static FIntv* F_new0() { return F_new(0.0, 0.0, NULL); }

static FIntv* F_clone1(FIntv* other)
{
    return F_new(other->lo, other->hi, other->next);
}

static Intv* I_new1(int64_t value, Intv* next)
{
    return I_new(value, value, next);
}

static Intv* I_new0() { return I_new(0, 0, NULL); }

static Intv* I_clone1(Intv* other)
{
    return I_new(other->lo, other->hi, other->next);
}

static Intv* I_clone(Intv* other)
{
    // deep clone all items in chain
    Intv* Io = I_clone1(other);
    Intv* I = Io;
    while (I->next) {
        I->next = I_clone1(I->next);
        I = I->next;
    }
    return Io;
}

static FIntv* F_clone(FIntv* other)
{
    // deep clone all items in chain
    FIntv* Io = F_clone1(other);
    FIntv* I = Io;
    while (I->next) {
        I->next = F_clone1(I->next);
        I = I->next;
    }
    return Io;
}

static void I_jins(Intv* intv, Intv* other)
{
    // you'll need to make a copy of other, since the next ptr is embeddded
    // if (not*intvp) {
    //     *intvp = I_new(other->lo, other->hi, NULL);
    //     // (*intvp)->lo = other->lo;
    //     // (*intvp)->hi = other->hi;
    //     return;
    // }
    // Intv* intv = *intvp;
    // I_desc(intv);
    Intv* orig = intv;
    while (intv and intv->next and intv->next->lo < other->lo)
        intv = intv->next;
    if (intv == orig and other->lo < intv->lo) {
        intv->next = I_clone1(intv);
        intv->lo = other->lo;
        intv->hi = other->hi;
    } else
        intv->next = I_new(other->lo, other->hi, intv->next);
    // I_desc(intv);
    // I_snap(intv);
    // I_desc(intv);

    // intv->next = PtrIist_with_next(other, intv->next);
}

static void F_jins(FIntv* fintv, FIntv* other)
{
    FIntv* orig = fintv;
    while (fintv and fintv->next and fintv->next->lo < other->lo)
        fintv = fintv->next;
    if (fintv == orig and other->lo < fintv->lo) {
        fintv->next = F_clone1(fintv);
        fintv->lo = other->lo;
        fintv->hi = other->hi;
    } else {
        fintv->next = F_new(other->lo, other->hi, fintv->next);
    }
}
// jins: "Just insert". Otherwise ins will call snap each time.
static void I_ins(Intv* intv, Intv* other)
{
    I_ins(intv, other);
    I_snap(intv);
}
static void F_ins(FIntv* fintv, FIntv* other)
{
    F_ins(fintv, other);
    F_snap(fintv);
}

static void I_del1(Intv* intv, int64_t value)
{
    // Get the particular subinterval which contains value
    // printf("deleting %lld from ", value);
    // I_print(intv);

    Intv* iv = I_find(intv, value);
    if (not iv) return; // value isn't in the list at all? get out
    if (iv->lo == value and iv->hi == value)
        // value is in a subinterval by itself
        *iv = *iv->next;
    // the old Intv will go out of scope and should be freed
    // TODO: dealloc(old iv)
    else if (iv->lo == value)
        iv->lo++; // value is the lower limit? increase it
    else if (iv->hi == value)
        iv->hi--; // value is the upper limit? decrease it
    else {
        // value is in betewen. You'll have to split this interval.
        Intv* newI = I_new(value + 1, iv->hi, iv->next);
        // NEW(Intv);
        // *newI = (Intv) { .lo = value + 1, .hi = iv->hi, .next = iv->next };
        iv->hi = value - 1;
        iv->next = newI; // PtrList_with_next(newI, iv->next);
    }
    // printf("after deletion ");
    // I_print(intv);
}

static void F_del1(FIntv* fintv, double value)
{
    // Get the particular subinterval which contains value
    // printf("deleting %lld from ", value);
    // I_print(intv);

    FIntv* fiv = F_find(fintv, value);
    if (not fiv) return; // value isn't in the list at all? get out
    if (fiv->lo == value and fiv->hi == value)
        // value is in a subinterval by itself
        *fiv = *fiv->next;
    // the old Intv will go out of scope and should be freed
    // TODO: dealloc(old iv)
    else if (fiv->lo == value)
        fiv->lo = nextafter(fiv->lo, DBL_MAX);
    // value is the lower limit? increase it
    else if (fiv->hi == value)
        fiv->hi = nextafter(fiv->hi, -DBL_MAX);
    // value is the upper limit? decrease it
    else {
        // value is in betewen. You'll have to split this interval.
        FIntv* newF = F_new(nextafter(value, DBL_MAX), fiv->hi, fiv->next);
        // NEW(Intv);
        // *newI = (Intv) { .lo = value + 1, .hi = iv->hi, .next = iv->next };
        fiv->hi = nextafter(value, -DBL_MAX);
        fiv->next = newF; // PtrList_with_next(newI, iv->next);
    }
    // printf("after deletion ");
    // I_print(intv);
}

static void I_del(Intv* intv, Intv* other) {}

#define F_haz(F, v)                                                            \
    printf("%s %s %g\n", #F, F_find(F, v) ? "has" : "doesn't have", v);

#define I_haz(I, v)                                                            \
    printf("%s %s %d\n", #I, I_find(I, v) ? "has" : "doesn't have", v);

Intv* parse(const char* str)
{
    Intv* intv = NULL;
    // const char* buf = "[-2:2, 5:6, 7:9, 8:10]";
    char* ptr = strchr(str, '[');
    if (not ptr) {
        printf("parser error\n");
        return NULL;
    } else {
        ptr++;
    }
    int64_t a, b;
    while (ptr and *ptr) {
        a = strtoll(ptr, &ptr, 0);
        if (*ptr != ':') {
            printf("parser error: expecting ':'\n");
            break;
        }
        ptr++;
        b = strtoll(ptr, &ptr, 0);

        // printf("found %lld:%lld\n", a, b);
        if (not intv) {
            intv = I_new(a, b, NULL);
        } else {
            Intv tmp = { .lo = a, .hi = b };
            I_jins(intv, &tmp);
        }

        while (*ptr == ' ') ptr++;
        if (*ptr == ']') break;
        if (*ptr == ',')
            ptr++;
        else {
            printf("parser error: unexpected '%c'\n", *ptr);
            break;
        }
    }
    // printf("parsed: ");
    // I_print(intv);

    I_snap(intv);
    // printf("snapped: ");
    // I_print(intv);
    return intv;
}

char* convertBase(int64_t n, int k, char* a)
{
    int j, i = 0, sign = 0;
    if (n == 0) a[i--] = '0';
    if (n < 0) {
        sign = -1;
        n = -n;
    }
    while (n > 0) {
        j = n % k;
        printf("%2d %c\n", j, j < 10 ? j + '0' : j - 10 + 'A');
        if (j < 10)
            a[i] = j + '0';
        else
            a[i] = j - 10 + 'A';
        n = n / k;
        i--;
    }
    if (sign == -1) a[i--] = '-';
    // a[i] = 0;
    return a + i + 1;
}

int main()
{
    // printf("%.55g ->\n  %.55g\n", 0.0, nextafter(10.0e213, DBL_MAX));
    I_print(parse("[-2:2, 5:6, 7:9, 8:10]"));
    // char* smp = "Y2p0IJ32e8E7";
    // uint64_t smpul = strtoull(smp, NULL, 36);
    // printf("%llu %llu\n", smpul, UINT64_MAX - smpul);

    // char buf[128] = {};
    // // memset(buf, '.', 127);
    // char* p = convertBase(INT64_MIN + 1, 36, buf + 126);
    // printf("%p %p %p\n", p, buf, buf + 127);
    // printf("%s\n", p);

    FIntv a[1] = { { .lo = 3.3, .hi = 5.1 } };
    FIntv b[1] = { { .lo = -5.4, .hi = -3.1 } };
    FIntv c[1] = { { .lo = -3.14, .hi = -3.04 } };
    FIntv d[1] = { { .lo = 10.03, .hi = 11.1234 } };
    FIntv e[1] = { { .lo = 19, .hi = 40.5 } };
    FIntv f[1] = { { .lo = 7.5, .hi = 10.03 } };
    FIntv* intv = F_new0();

    F_jins(intv, a);
    F_jins(intv, b);
    F_jins(intv, c);
    F_jins(intv, d);
    F_jins(intv, e);
    F_jins(intv, f);
    F_snap(intv); // TODO: probably move snap into ins or user will forget to
                  // call it every time. but this is good for perf if you call
                  // it after inserting all you want.

    // I_haz(a, 6);
    F_desc(intv);
    F_haz(intv, 7.0);

    F_del1(intv, 7);
    F_desc(intv);
    F_haz(intv, 7.0);
    F_mul(intv, c);
    F_desc(intv);
    // I_add(intv, f);
    // I_desc(intv);
    // I_snap(intv);
    // I_desc(intv);

    // int64_t i = 1UL << 62, j = 1UL << 62;
    // printf("%d\n", canoverflow_add(i, j));
    // printf("%d\n", canoverflow_mul(i, j));
    // printf("%d\n", canoverflow_sub(i, j));
    // printf("%lld %lld %lld %lld\n", i, j, i + j, signed_saturated_add(i, j));

    // for (int64_t i = 1; i < INT64_MAX; i *= 2) {
    //     printf("%lld %20lld %20lld %20lld\n", 512LL, i, i * 512,
    //         signed_saturated_mul(i, 512));
    //     if (canoverflow_mul(512, i)) {
    //         printf("./%s:%d:%d: runtime error: %lld * %lld has overflowed\n",
    //             __FILE__, 315, 61, 512LL, i);
    //         break;
    //     }
    // }
    // printf("%d\n", not not I_which(intv, 12));
    // printf("%d\n", not not I_which(intv, 7));
    // printf("%d\n", not not I_which(intv, 3));
    return 0;
}
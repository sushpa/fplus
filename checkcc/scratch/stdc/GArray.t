#if !defined(T)
#error "`T` not defined for GArray"
#endif

#if !defined(I)
#error "`I` not defined for GArray"
#endif

typedef void* VoidPtr;
#define T VoidPtr
#define I 32
#define bool char

#define uint32_t unsigned int

#define uint_(x) uint##x##_t
#define uint(x) uint_(x)

#define JOIN4_(a, b, c, d) a##b##c##d
#define JOIN4(a, b, c, d) JOIN4_(a, b, c, d)
#define JOIN3_(a, b, c) a##b##c
#define JOIN3(a, b, c) JOIN3_(a, b, c)
#define JOIN2_(a, b) a##b
#define JOIN2(a, b) JOIN2_(a, b)

// #define INTTYPE JOIN3(uint, I, _t)

#define GArray(T, I) JOIN3(GArray, T, I)

typedef struct GArray(T, I) GArray(T, I);

struct GArray(T, I)
{
    T* ref;
    // union {
    //     char* c;
    //     int* i;
    //     long long* I;
    //     unsigned int* u;
    //     unsigned long long* U;
    //     double* f;
    //     struct {
    //         double ___gar, ___bage;
    //     } * S; // increments 16 B
    //     void* p;
    // } ref;
    // TODO: step should be int
    uint(I) count, alloc, step, pos;
    // start stop are handled by setting the ref and count anyway for a
    // slice
};

T GArray_get(T, I)(GArray(T, I) * self, uint(I) idx)
{
    return self->ref[idx];
}

#define Array(T) GArray(T, 32)
#define ArrayL(T) GArray(sizeof(T), 64)
#define ArrayN(T, D) GArrayN(sizeof(T), D, 32)
#define ArrayNL(T, D) GArrayN(sizeof(T), D, 64)
#define Slice(T) GSlice(sizeof(T), 32)
#define SliceN(T, D) GSliceN(sizeof(T), D, 32)
#define SliceL(T) GSlice(sizeof(T), 64)
#define SliceNL(T, D) GSliceN(sizeof(T), D, 64)
#define iter(AorS) iter##AorS
#define iterArray(i, arr) for (I i = 0; i < arr->count; i += 1)
#define iterSlice(i, sli)                                                  \
    for (I i = sli->start; i < arr->end; i += sli->step)
// ^ use these heavily in implementation of below stuff
// to make same methods available in both Array and Slice.
////////////////////////////////////////////////////////////////////////////////

static void push(T item) {}

static void justPush(T item) {}

static T* pushp() {}

static T pop() {}

static void concat(GArray(T, I) array) {}

static void insertAt(uint(I) index) {}

static void insertBefore(T item) {}

static void insertAfter(T item) {}

static void removeAt(uint(I) index) {}

static void removeItem(T item) {}

static bool contains(T item) {}

static uint(I) countItem(T item) {}

static bool any() {}

static bool all() {}

static uint(I) count() {}

static GArray(T, I) copy() {}

static uint(I) count_filter() {}

static bool any_filter() {}

static bool all_filter() {}

static GArray(T, I) copy_filter() {}
static T min_filter() {}
static T max_filter() {}
static T mean_filter() {}
static T stddev_filter() {} // etc.
static void write() {}
// write textual repr to file, to string, to screen,
// whatever -- basically calls write() of elements
// write(self[0], ", ", self[4]) etc

static void write_filter() {}

static T getAt(uint(I) index) {}

static T getSafelyAt(uint(I) index) {}

static bool inbounds(uint(I) index) {}

static uint(I) realIndex(uint(I) index) /* +ve, in n-d all dims collapsed*/
{
}

static void setAt(uint(I) index) {}

static void setSafelyAt(uint(I) index) {}

static void setItemsInSlice(
    Slice slice, GArray(T, I) otherArray, uint(I) offset)
{
}
// offset of start index between this array and other. same slice info is
// used

static uint(I) firstIndex(T item) {}

static uint(I) lastIndex(T item) {}

// NO: this should be defined for the element @
static uint(I) compare(uint(I) index1, uint(I) index2) {}
return T##_compare(_get(self, index1), _get(self, index2))

    static void swap(uint(I) index1, uint(I) index2)
{
}

static void clear() {}

static void sortQuick() {}

static void sortMerge() {}

static void shuffleKnuth() {}

static void searchBinary(T value) {}

static bool equals(GArray(T, I) otherArray) {}
// filter/map -- but it should be impl as a macro. besides funcs r elemental

static String joinToString(String sep) {}

static void set() {}
static void set_filter() {}

// and _filter versions for all below
static void add(GArray(T, I) otherArray)
{
    assert(self->count == otherArray->count);
    for (uint(I) i = 0; i < self->count; i++)
        self[i] += otherArray[i];
}

static void sub(GArray(T, I) otherArray) {}

void mul(GArray(T, I) otherArray) {}

void div(GArray(T, I) otherArray) {}

void pow(GArray(T, I) otherArray) {}

void mod(GArray(T, I) otherArray) {}

void add1(T value) {}

void sub1(T value) {}

void mul1(T value) {}

void div1(T value) {}

void pow1(T value) {}

void mod1(T value) {}

////////////////////////////////////////////////////////////////////////////////

/*
////////////////////////////////////////////////////////////////////////////////
@ GArrayV
> S, I
^ GArray(S, I)
% Array(int(I)) dlen
*/

////////////////////////////////////////////////////////////////////////////////
// @GSlice > T, I % GArray(T, I) * ref % int(I) start, stop, step,
//     pos

//         - reset

//         - hasNext

//         - next
//         // slice should generally have all read methods of arrays. what
//         // about write methods?
//         - get
//         ////////////////////////////////////////////////////////////////////////////////
//         @GSliceN
//     > S,
//     D, I % GArrayN(S, D, I) * ref % int(I) start, stop, step, pos

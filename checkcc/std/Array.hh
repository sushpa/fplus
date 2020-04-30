
#define mutop(op)                                                          \
    void operator op(const Array<T>& rhs)                                  \
    {                                                                      \
        for (auto l = start, r = rhs.start; l < stop; l++, r++)            \
            *l op* r;                                                      \
    }
#define MUT_OPS mutop(=) mutop(+=) mutop(-=) mutop(*=) mutop(/=) mutop(%=)

typedef long long Int; // must be ssize_t
// Int: int, size of platform (32/64)
// Int4, Int8 to force
// similarly Nat, Nat4, Nat8
// Real is always double. use Real4 for single.
// Char is char, Byte is uchar

template <class T> struct Array {
    T *start, *stop, *end;
    T& operator[](Int idx) { return start[idx]; }
    MUT_OPS

    Array<T>(Int size = 4)
    {
        if (size) resize(size);
    }
    // Array<T>() { start = stop = end = NULL; }
    Int alloc() const { return end - start; }
    void grow() { resize(2 * alloc()); }
    void snap() { stop = end; }
    void resize(Int newSize)
    {
        newSize *= sizeof(T);
        start = realloc(start, newSize);
        end = start + newSize;
        if (end < stop) stop = end;
    }
    Int avail() const { return end - stop; }
    bool full() const { return end <= stop; }
    void grow(Int bySize) { resize(bySize + end - start); }
};

static_assert(sizeof(Array<double>) == 24, "");

template <class T> static Int len(const Array<T>& arr)
{
    return arr.stop - arr.start;
}

template <class T> static void pushf(Array<T>& arr, T item)
{
    *arr.stop++ = item;
}

template <class T> static void push(Array<T>& arr, T item)
{
    if (arr.full()) arr.grow();
    pushf(arr, item);
}
template <class T> static T* pushp(Array<T>& arr) { return arr.stop++; }
template <class T> static T pop(Array<T>& arr) { return *arr.stop--; }
template <class T> static T top(const Array<T>& arr) { return *arr.stop; }

template <class T> static void concat(Array<T>& arr, const Array<T>& arr2)
{
    // TODO: grow by power of 2 instead of resizing to nsz so that
    // repeated calls to concat do not each result in a realloc.
    Int nsz = len(arr) + len(arr2);
    if (nsz > arr.alloc()) arr.resize(nsz);
    memcpy(arr.stop, arr2.start, sizeof(T) * len(arr2));
}

template <class T> static void insertAt(Array<T>& arr, Int index) {}

template <class T> static void insertBefore(Array<T>& arr, T item) {}

template <class T> static void insertAfter(Array<T>& arr, T item) {}

template <class T> static void removeAt(Array<T>& arr, Int index) {}

template <class T> static void removeItem(Array<T>& arr, T item) {}

template <class T> static bool contains(const Array<T>& arr, const T item)
{
    bool ans;
    Array_any_filter(arr, arr[i] == item, ans);
    return ans;
}

//  define equal() for each type to override this
// (if that type uses a different definition of equals)
// template <class T> static bool equal(T lhs, T rhs) { return lhs == rhs; }

// template <class T> static Int countItem(Array<T>& arr, T item) {}

// turns out fine for any dim arr. for sli there should be a different
// one. SIMDize whatever etc. do it here.

#define Array_count_filter(arr, filter, ans)                               \
    ans = 0;                                                               \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) ans++;

#define Array_any_filter(arr, filter, ans)                                 \
    ans = false;                                                           \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) {                                                      \
            ans = true;                                                    \
            break;                                                         \
        }

#define Array_all_filter(arr, filter, ans)                                 \
    Array_any_filter(arr, not(filter), ans);                               \
    ans = not ans;
//     ans = true;                                                            \
//     for (Int i = 0; i < arr.len; i++)                                   \
//         if (not(filter)) {                                                 \
//             ans = false;                                                   \
//             break;                                                         \
//         }

template <class T> static bool Array_any(Array<T>& arr)
{
    bool ans;
    Array_any_filter(arr, arr[i], ans);
    return ans;
}

template <class T> static bool Array_all(Array<T>& arr)
{
    bool ans;
    Array_all_filter(arr, arr[i], ans);
    return ans;
}

template <class T> static Int count(Array<T>& arr)
{
    Int ans;
    Array_count_filter(arr, true, ans);
    return ans;
}

#define Array_set_filter(arr, filter, val)                                 \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) arr[i] = val;

// apply an expr elementwise, return arr of same size as source with
// the result of expr on each element.
// the compiler should append `[i]` to all idents INSIDE expr which are the
// arr's name.

#define Array_map_filter(arr, expr, filter, ans)                           \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) Array_push(ans, expr);

#define Array_map(arr, expr, ans)                                          \
    for (Int i = 0; i < arr.len; i++)                                      \
        ans[i] = expr;

// like map, but instead of returning arr, returns a scalar
// useful for reductions like min,max,sum etc.
// func is sum,min,max,stddev,mean,prod,...
// that func should be defined for scalars.

#define Array_reduce_filter(arr, func, filter, ans)                        \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) ans = func(ans, arr[i]);

#define Array_reduce(arr, func, ans)                                       \
    Array_reduce_filter(arr, func, true, ans)

// apply a stmt elementwise to an arr, doesn't return anything
// stmt should include all temp vars needed, defined based on i.
// for instance operations on arr slices etc. can be made element ops
// within a single loop (atleast for 1D). indexes for each arr referred to
// can be computed as a func/offset of i and included in stmt.

// this one is for part of the 1D arr, e.g. sli or by range
// may need separate ones like this for 2D, 3D, ...
#define Array_do_filter_part(arr, filter, len, stmt)                       \
    for (Int i = 0; i < len; i++)                                          \
        if (filter) {                                                      \
            stmt;                                                          \
        }

#define Array_do_filter(arr, filter, stmt)                                 \
    Array_do_filter_part(arr, filter, arr.len, stmt)

#define Array_do(arr, stmt) Array_do_filter(arr, true, stmt)

// should Array_copy create a new arr? (in general all these macros)
// e.g. var x = copy(b) needs a new arr.
// but var x = zeros(...); x = copy(b) does not...
#define Array_copy_filter(arr, filter, ans)                                \
    ans = Array_newlike(arr); /* sets alloc but not len */                 \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) Array_push(ans, arr[i]);

#define memdup(start, sz)                                                  \
    {                                                                      \
        auto tmp = start;                                                  \
        start = malloc(sz);                                                \
        memcpy(start, tmp, sz); /*FIXME */                                 \
    }

template <class T> static Array<T> Array_copy(Array<T>& arr)
{
    auto ans = arr;
    memdup(ans.start, ans.alloc * sizeof(T));
    return ans;
}

// template <class T> static Int count_filter(Array<T>& arr) {}

// the _ filter ones should be macros!
// template <class T> static bool any_filter(Array<T>& arr) {}

// template <class T> static bool all_filter(Array<T>& arr) {}

#define Array_min_filter(arr, filter, ans)                                 \
    ans = MAXPOSBL(T);                                                     \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) ans = min(ans, arr[i]);

#define Array_max_filter(arr, filter, ans)                                 \
    ans = MINPOSBL(T);                                                     \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) ans = max(ans, arr[i]);

// #define Array_mean_filter(arr, func, filter, ans)                        \
//     {                                                                      \
//         ans = 0;                                                           \
//         Int c = 0;                                                           \
//         for (Int i = 0; i < arr.len; i++)                               \
//             if (filter) {                                                  \
//                 ans += arr[i];                                          \
//                 c++;                                                       \
//             }                                                              \
//         if (c) ans /= c;                                                   \
//     }

// hopefully the C compiler will merge the two loops
#define Array_mean_filter(arr, filter, ans)                                \
    {                                                                      \
        Int c = 0;                                                         \
        Array_sum_filter(arr, filter, ans);                                \
        Array_count_filter(arr, filter, c);                                \
        if (c) ans /= c;                                                   \
    }

#define Array_stddev_filter(arr, filter, ans)                              \
    ans = 0;                                                               \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) ans = ...(ans, arr[i]);

#define Array_sum_filter(arr, filter, ans)                                 \
    ans = 0;                                                               \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) ans += arr[i];

#define Array_prod_filter(arr, filter, ans)                                \
    ans = 1;                                                               \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) ans *= arr[i];

#define Array_rms_filter(arr, filter, ans)                                 \
    ans = 0;                                                               \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) ans = ...(ans, arr[i]);

#define Array_min(arr, ans) Array_min_filter(arr, true, ans)
#define Array_max(arr, ans) Array_max_filter(arr, true, ans)
#define Array_mean(arr, ans) Array_mean_filter(arr, true, ans)
#define Array_stddev(arr, ans) Array_stddev_filter(arr, true, ans)
#define Array_sum(arr, ans) Array_sum_filter(arr, true, ans)
#define Array_prod(arr, ans) Array_prod_filter(arr, true, ans)
#define Array_rms(arr, ans) Array_rms_filter(arr, true, ans)

// template <class T> static T Array_max_filter(Array<T>& arr) {}
// template <class T> static T Array_mean_filter(Array<T>& arr) {}
// template <class T> static T Array_stddev_filter(Array<T>& arr) {} // etc.
template <class T> static void write(Array<T>& arr) {}
// write textual repr to file, to string, to screen,
// whatever -- basically calls write() of elements
// write(self[0], ", ", self[4]) etc

template <class T> static void Array_write_filter(Array<T>& arr) {}

// template <class T> static T getAt(Int index) {}

// template <class T> static T getSafelyAt(Int index) {}

template <class T> static bool inbounds(Array<T>& arr, Int index)
{
    return index < arr.len;
}

template <class T>
static Int realIndex(
    Array<T>& arr, Int index) /* +ve, in n-d all dims collapsed*/
{
    return index >= 0 ? index : -index;
}

template <class T> static void setAt(Int index) {}

template <class T> static void setSafelyAt(Int index) {}

template <class T>
static void setItemsInSlice(Slice sli, Array<T> otherArray, Int offset)
{
}
// offset of start index between this arr and other. same sli info is
// used

// template <class T> static Int firstIndex(T item) {}

// template <class T> static Int lastIndex(T item) {}

// NO: this should be defined for the element @
// template <class T> static Int compare(Int index1,
// Int index2) {} return T##_compare(_get(self, index1), _get(self,
// index2))

template <class T> static void swap(Array<T>& arr, Int index1, Int index2)
{
    auto tmp = arr[index2];
    arr[index2] = arr[index1];
    arr[index1] = tmp;
}

template <class T> static void clear(Array<T>& arr) { arr.len = 0; }

template <class T> static void sortQuick(Array<T>& arr) {}

template <class T> static void sortMerge(Array<T>& arr) {}

template <class T> static void shuffleKnuth(Array<T>& arr) {}

template <class T> static Int searchBinary(Array<T>& arr, T value) {}

template <class T> static bool equals(Array<T>& arr, Array<T> otherArray) {}
// filter/map -- but it should be impl as a macro. besides funcs r elemental

template <class T> static String joinToString(Array<T>& arr, String sep) {}

// template <class T> static void set() {}
// template <class T> static void set_filter() {}
template <class T> static void hash() {}
template <class T> static void equals_filter() {}

// and _filter versions for all below
// template <class T> static void add(Array<T,Int> otherArray)
// {
//     assert(self->count == otherArray->count);
//     for (Int i = 0; i < self->count; i++)
//         self[i] += otherArray[i];
// }

// these are all funcsmacros for the case where filtered self-op is reqd
// e.g. arr[arr < 1] += 7.5 or
// arr[arr>7] *= arr2[arr<7] // should sizes matter here?
#define mutop(lhs, op, rhs, filter)                                        \
    for (Int i = 0; i < lhs.len; i++)                                      \
        if (filter) lhs[i] op rhs;

// rhs is only evaluated once here
#define mutop1(lhs, op, rhs, filter)                                       \
    {                                                                      \
        auto tmp = rhs;                                                    \
        for (Int i = 0; i < lhs.len; i++)                                  \
            if (filter) lhs[i] op tmp;                                     \
    }

#define Array_set_filter(arr, filter, otherArray)                          \
    mutop(arr, =, otherArray[i])
#define Array_add_filter(arr, filter, otherArray)                          \
    mutop(arr, +=, otherArray[i])
#define Array_sub_filter(arr, filter, otherArray)                          \
    mutop(arr, -=, otherArray[i])
#define Array_mul_filter(arr, filter, otherArray)                          \
    mutop(arr, *=, otherArray[i])
#define Array_div_filter(arr, filter, otherArray)                          \
    mutop(arr, /=, otherArray[i])
#define Array_mod_filter(arr, filter, otherArray)                          \
    mutop(arr, %=, otherArray[i])
#define Array_pow_filter(arr, filter, otherArray)                          \
    for (Int i = 0; i < arr.len; i++)                                      \
        if (filter) arr[i] = mpow(arr[i], otherArray[i]);

// the funcs below will evaluate value only once.
// so arr[arr>4] += random() or arr += random()
// will set the same random value to all arr elements that match.
// if you want fresh eval of rhs for each element, then do
// var rnds = randoms(arr.shape)
// arr += rnds
// or (but this cant avoid copy on tmp)
// var tmp = arr[arr>4]
// var rnds = randoms(tmp.shape)
// arr[arr>4] += rnds
// - NOOOOOOOOOOOOO
// TODO: Int think even the mutop1 funcs should eval rhs every time!
// let the user create a tmp scalar when he needs it. otherwise he has to
// create a tmp arr to get around the caching of rhs.
// so this:
// arr[arr>4] += random()
// will use a new random value each time.
#define Array_set1_filter(arr, filter, value) mutop1(arr, =, value)
#define Array_add1_filter(arr, filter, value) mutop1(arr, +=, value)
#define Array_sub1_filter(arr, filter, value) mutop1(arr, -=, value)
#define Array_mul1_filter(arr, filter, value) mutop1(arr, *=, value)
#define Array_div1_filter(arr, filter, value) mutop1(arr, /=, value)
#define Array_mod1_filter(arr, filter, value) mutop1(arr, %=, value)
#define Array_pow1_filter(arr, filter, value)                              \
    {                                                                      \
        auto tmp = value;                                                  \
        for (Int i = 0; i < arr.len; i++)                                  \
            if (filter) arr[i] = mpow(arr[i], tmp);                        \
    }

#define Array_set(arr, value) Array_set_filter(arr, true, value)
#define Array_add(arr, value) Array_add_filter(arr, true, value)
#define Array_sub(arr, value) Array_sub_filter(arr, true, value)
#define Array_mul(arr, value) Array_mul_filter(arr, true, value)
#define Array_div(arr, value) Array_div_filter(arr, true, value)
#define Array_mod(arr, value) Array_mod_filter(arr, true, value)
#define Array_pow(arr, value) Array_pow_filter(arr, true, value)

#define Array_set1(arr, value) Array_set1_filter(arr, true, value)
#define Array_add1(arr, value) Array_add1_filter(arr, true, value)
#define Array_sub1(arr, value) Array_sub1_filter(arr, true, value)
#define Array_mul1(arr, value) Array_mul1_filter(arr, true, value)
#define Array_div1(arr, value) Array_div1_filter(arr, true, value)
#define Array_mod1(arr, value) Array_mod1_filter(arr, true, value)
#define Array_pow1(arr, value) Array_pow1_filter(arr, true, value)

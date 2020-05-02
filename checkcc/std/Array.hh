// how to handle something like:
// sum(2 + arr[arr > 0] + arr[mx < 4])

#define mutop(op)                                                          \
    void operator op(const Array<T>& rhs)                                  \
    {                                                                      \
        /* assert(len(rhs) == len(*this));*/                               \
        for (T* l = start, r = rhs.start; l < stop; l++, r++)              \
            *l op* r;                                                      \
    }                                                                      \
    void operator op(const T& rhs)                                         \
    {                                                                      \
        for (T* l = start; l < stop; l++)                                  \
            *l op rhs;                                                     \
    }

#define MUT_OPS mutop(=) mutop(+=) mutop(-=) mutop(*=) mutop(/=) mutop(%=)

typedef long long Int; // must be ssize_t
// Int: int, size of platform (32/64)
// Int4, Int8 to force
// similarly Nat, Nat4, Nat8
// Real is always double. use Real4 for single.
// Char is char, Byte is uchar
template <class T> T* memdup(T* ptr, Int size)
{
    size *= sizeof(T);
    T* buf = (T*)malloc(size);
    memcpy(buf, ptr, size);
    return buf;
}

void print(Int x) { printf("%lld", x); }
void print(char x)
{
    if (isgraph(x))
        printf("'%c'", x);
    else
        printf("'\\%u'", x);
}

template <class T> struct Array {
    T *start, *stop, *end;
    T& operator[](Int idx) { return start[idx]; }
    MUT_OPS

    Array<T>(Int size = 4, bool useup = false)
    {
        start = stop = end = NULL;
        if (size) growto(size);
        if (useup) snap();
    }
    Array<T>(T* array, Int size, bool justRef = false)
    {
        start = justRef ? array : memdup(array, size);
        stop = start + size;
        end = justRef ? NULL : stop;
    }
    ~Array<T>()
    {
        // printf("%p %p %lld\n", start, end, end - start);
        // if (end) free(start);
    }
    // Array<T>() { start = stop = end = NULL; }
    Int alloc() const { return end - start; }
    void grow() { growto(2 * alloc()); }
    void snap() { stop = end; }
    void resize(Int size)
    {
        growto(size);
        snap()
    }
    void growto(Int size)
    {
        start = (T*)realloc(start, size * sizeof(T));
        end = start + size;
        if (end < stop) stop = end;
    }
    Int avail() const { return end - stop; }
    bool full() const { return end <= stop; }
    void growby(Int size) { growto(alloc() + size); }
};

Array<Int> randoms(Int size)
{
    auto vec = Array<Int>(size);
    for (Int* l = vec.start; l < vec.stop; l++)
        *l = random();
    return vec;
}

template <class T> T sum(Array<T> vec)
{
    T s = 0;
    for (auto l = vec.start; l < vec.stop; l++)
        s += *l;
    return s;
}

template <class T> static void print(const Array<T>& vec)
{
    static const char* sep[2] = { ", ", "[ " };
    for (auto l = vec.start; l < vec.stop; l++) {
        fputs(sep[l == vec.start], stdout);
        print(*l);
    }
    puts(" ]");
}
static_assert(sizeof(Array<double>) == 24, "");

template <class T> static Int len(const Array<T>& vec)
{
    return vec.stop - vec.start;
}

template <class T> static void pushf(Array<T>& vec, T item)
{
    *vec.stop++ = item;
}

template <class T> static void push(Array<T>& vec, T item)
{
    if (vec.full()) vec.grow();
    pushf(vec, item);
}
template <class T> static T* pushp(Array<T>& vec) { return vec.stop++; }
template <class T> static T pop(Array<T>& vec) { return *vec.stop--; }
template <class T> static T top(const Array<T>& vec) { return *vec.stop; }

template <class T> static void concat(Array<T>& vec, const Array<T>& arr2)
{
    // TODO: grow by power of 2 instead of resizing to nsz so that
    // repeated calls to concat do not each result in a realloc.
    Int nsz = len(vec) + len(arr2);
    if (nsz > vec.alloc()) vec.resize(nsz);
    memcpy(vec.stop, arr2.start, sizeof(T) * len(arr2));
}

template <class T> static void insertAt(Array<T>& vec, Int index) {}

template <class T> static void insertBefore(Array<T>& vec, T item) {}

template <class T> static void insertAfter(Array<T>& vec, T item) {}

template <class T> static void removeAt(Array<T>& vec, Int index) {}

template <class T> static void removeItem(Array<T>& vec, T item) {}

#define Array_count_filter(vec, filter, ans)                               \
    ans = 0;                                                               \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) ans++;

#define Array_any_filter(vec, filter, ans)                                 \
    ans = false;                                                           \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) {                                                      \
            ans = true;                                                    \
            break;                                                         \
        }

#define Array_all_filter(vec, filter, ans)                                 \
    Array_any_filter(vec, not(filter), ans);                               \
    ans = not ans;
//     ans = true;                                                            \
//     for (Int i = 0; i < vec.len; i++)                                   \
//         if (not(filter)) {                                                 \
//             ans = false;                                                   \
//             break;                                                         \
//         }

template <class T> static bool contains(const Array<T>& vec, const T item)
{
    bool ans;
    Array_any_filter(vec, vec[i] == item, ans);
    return ans;
}

//  define equal() for each type to override this
// (if that type uses a different definition of equals)
// template <class T> static bool equal(T lhs, T rhs) { return lhs == rhs; }

// template <class T> static Int countItem(Array<T>& vec, T item) {}

// turns out fine for any dim vec. for sli there should be a different
// one. SIMDize whatever etc. do it here.

template <class T> static bool Array_any(Array<T>& vec)
{
    bool ans;
    Array_any_filter(vec, vec[i], ans);
    return ans;
}

template <class T> static bool Array_all(Array<T>& vec)
{
    bool ans;
    Array_all_filter(vec, vec[i], ans);
    return ans;
}

template <class T> static Int count(Array<T>& vec)
{
    Int ans;
    Array_count_filter(vec, true, ans);
    return ans;
}

// #define Array_set_filter(vec, filter, val)                                \
//     for (Int i = 0; i < vec.len; i++)                                      \
//         if (filter) vec[i] = val;

// apply an expr elementwise, return vec of same size as source with
// the result of expr on each element.
// the compiler should append `[i]` to all idents INSIDE expr which are the
// vec's name.

#define Array_map_filter(vec, expr, filter, ans)                           \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) Array_push(ans, expr);

#define Array_map(vec, expr, ans)                                          \
    for (Int i = 0; i < vec.len; i++)                                      \
        ans[i] = expr;

// like map, but instead of returning vec, returns a scalar
// useful for reductions like min,max,sum etc.
// func is sum,min,max,stddev,mean,prod,...
// that func should be defined for scalars.

#define Array_reduce_filter(vec, func, filter, ans)                        \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) ans = func(ans, vec[i]);

#define Array_reduce(vec, func, ans)                                       \
    Array_reduce_filter(vec, func, true, ans)

// apply a stmt elementwise to an vec, doesn't return anything
// stmt should include all temp vars needed, defined based on i.
// for instance operations on vec slices etc. can be made element ops
// within a single loop (atleast for 1D). indexes for each vec referred to
// can be computed as a func/offset of i and included in stmt.

// this one is for part of the 1D vec, e.g. sli or by range
// may need separate ones like this for 2D, 3D, ...
#define Array_do_filter_part(vec, filter, len, stmt)                       \
    for (Int i = 0; i < len; i++)                                          \
        if (filter) {                                                      \
            stmt;                                                          \
        }

#define Array_do_filter(vec, filter, stmt)                                 \
    Array_do_filter_part(vec, filter, vec.len, stmt)

#define Array_do(vec, stmt) Array_do_filter(vec, true, stmt)

// should Array_copy create a new vec? (in general all these macros)
// e.g. var x = copy(b) needs a new vec.
// but var x = zeros(...); x = copy(b) does not...
#define Array_copy_filter(vec, filter, ans)                                \
    ans = Array_newlike(vec); /* sets alloc but not len */                 \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) Array_push(ans, vec[i]);

#define memdup(start, sz)                                                  \
    {                                                                      \
        auto tmp = start;                                                  \
        start = malloc(sz);                                                \
        memcpy(start, tmp, sz); /*FIXME */                                 \
    }

template <class T> static Array<T> Array_copy(Array<T>& vec)
{
    auto ans = vec;
    memdup(ans.start, ans.alloc * sizeof(T));
    return ans;
}

// template <class T> static Int count_filter(Array<T>& vec) {}

// the _ filter ones should be macros!
// template <class T> static bool any_filter(Array<T>& vec) {}

// template <class T> static bool all_filter(Array<T>& vec) {}

#define Array_min_filter(vec, filter, ans, expr)                           \
    ans = MAXPOSBL(T);                                                     \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) ans = min(ans, expr);

// generalized for any expr e.g. 2 + 3 * vec[i]. all other reductions should
// also be generalized. for the usual case, default expr is: vec[i]
#define Array_max_filter(vec, filter, ans, expr)                           \
    ans = MINPOSBL(T);                                                     \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) ans = max(ans, expr);

// #define Array_mean_filter(vec, func, filter, ans)                        \
//     {                                                                      \
//         ans = 0;                                                           \
//         Int c = 0;                                                           \
//         for (Int i = 0; i < vec.len; i++)                               \
//             if (filter) {                                                  \
//                 ans += vec[i];                                          \
//                 c++;                                                       \
//             }                                                              \
//         if (c) ans /= c;                                                   \
//     }

// hopefully the C compiler will merge the two loops
#define Array_mean_filter(vec, filter, ans)                                \
    {                                                                      \
        Int c = 0;                                                         \
        Array_sum_filter(vec, filter, ans);                                \
        Array_count_filter(vec, filter, c);                                \
        if (c) ans /= c;                                                   \
    }

#define Array_stddev_filter(vec, filter, ans)                              \
    ans = 0;                                                               \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) ans = ...(ans, vec[i]);

#define Array_sum_filter(vec, filter, ans)                                 \
    ans = 0;                                                               \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) ans += vec[i];

#define Array_prod_filter(vec, filter, ans)                                \
    ans = 1;                                                               \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) ans *= vec[i];

#define Array_rms_filter(vec, filter, ans)                                 \
    ans = 0;                                                               \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) ans = ...(ans, vec[i]);

#define Array_min(vec, ans) Array_min_filter(vec, true, ans)
#define Array_max(vec, ans) Array_max_filter(vec, true, ans)
#define Array_mean(vec, ans) Array_mean_filter(vec, true, ans)
#define Array_stddev(vec, ans) Array_stddev_filter(vec, true, ans)
#define Array_sum(vec, ans) Array_sum_filter(vec, true, ans)
#define Array_prod(vec, ans) Array_prod_filter(vec, true, ans)
#define Array_rms(vec, ans) Array_rms_filter(vec, true, ans)

// template <class T> static T Array_max_filter(Array<T>& vec) {}
// template <class T> static T Array_mean_filter(Array<T>& vec) {}
// template <class T> static T Array_stddev_filter(Array<T>& vec) {} //
// etc.
template <class T> static void write(Array<T>& vec) {}
// write textual repr to file, to string, to screen,
// whatever -- basically calls write() of elements
// write(self[0], ", ", self[4]) etc

template <class T> static void Array_write_filter(Array<T>& vec) {}

// template <class T> static T getAt(Int index) {}

// template <class T> static T getSafelyAt(Int index) {}

template <class T> static bool inbounds(Array<T>& vec, Int index)
{
    return index < vec.len;
}

template <class T>
static Int realIndex(
    Array<T>& vec, Int index) /* +ve, in n-d all dims collapsed*/
{
    return index >= 0 ? index : index + vec.stop - vec.start;
}

template <class T> static void setAt(Int index) {}

template <class T> static void setSafelyAt(Int index) {}

// template <class T>
// static void setItemsInSlice(Slice sli, Array<T> otherArray, Int offset)
// {
// }
// offset of start index between this vec and other. same sli info is
// used

// template <class T> static Int firstIndex(T item) {}

// template <class T> static Int lastIndex(T item) {}

// NO: this should be defined for the element @
// template <class T> static Int compare(Int index1,
// Int index2) {} return T##_compare(_get(self, index1), _get(self,
// index2))

template <class T> static void swap(Array<T>& vec, Int index1, Int index2)
{
    T tmp = vec[index2];
    vec[index2] = vec[index1];
    vec[index1] = tmp;
}

template <class T> static void clear(Array<T>& vec) { vec.len = 0; }

template <class T> static void sortQuick(Array<T>& vec) {}

template <class T> static void sortMerge(Array<T>& vec) {}

template <class T> static void shuffleKnuth(Array<T>& vec) {}

template <class T> static Int searchBinary(Array<T>& vec, T value) {}

template <class T> static bool equals(Array<T>& vec, Array<T> otherArray) {}
// filter/map -- but it should be impl as a macro. besides funcs r elemental

// template <class T> static String joinToString(Array<T>& vec, String sep)
// {}

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
// e.g. vec[vec < 1] += 7.5 or
// vec[vec>7] *= arr2[vec<7] // should sizes matter here?
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

#define Array_set_filter(vec, filter, otherArray)                          \
    mutop(vec, =, otherArray[i])
#define Array_add_filter(vec, filter, otherArray)                          \
    mutop(vec, +=, otherArray[i])
#define Array_sub_filter(vec, filter, otherArray)                          \
    mutop(vec, -=, otherArray[i])
#define Array_mul_filter(vec, filter, otherArray)                          \
    mutop(vec, *=, otherArray[i])
#define Array_div_filter(vec, filter, otherArray)                          \
    mutop(vec, /=, otherArray[i])
#define Array_mod_filter(vec, filter, otherArray)                          \
    mutop(vec, %=, otherArray[i])
#define Array_pow_filter(vec, filter, otherArray)                          \
    for (Int i = 0; i < vec.len; i++)                                      \
        if (filter) vec[i] = mpow(vec[i], otherArray[i]);

// the funcs below will evaluate value only once.
// so vec[vec>4] += random() or vec += random()
// will set the same random value to all vec elements that match.
// if you want fresh eval of rhs for each element, then do
// var rnds = randoms(vec.shape)
// vec += rnds
// or (but this cant avoid copy on tmp)
// var tmp = vec[vec>4]
// var rnds = randoms(tmp.shape)
// vec[vec>4] += rnds
// - NOOOOOOOOOOOOO
// TODO: Int think even the mutop1 funcs should eval rhs every time!
// let the user create a tmp scalar when he needs it. otherwise he has to
// create a tmp vec to get around the caching of rhs.
// so this:
// vec[vec>4] += random()
// will use a new random value each time.
#define Array_set1_filter(vec, filter, value) mutop1(vec, =, value)
#define Array_add1_filter(vec, filter, value) mutop1(vec, +=, value)
#define Array_sub1_filter(vec, filter, value) mutop1(vec, -=, value)
#define Array_mul1_filter(vec, filter, value) mutop1(vec, *=, value)
#define Array_div1_filter(vec, filter, value) mutop1(vec, /=, value)
#define Array_mod1_filter(vec, filter, value) mutop1(vec, %=, value)
#define Array_pow1_filter(vec, filter, value)                              \
    {                                                                      \
        auto tmp = value;                                                  \
        for (Int i = 0; i < vec.len; i++)                                  \
            if (filter) vec[i] = mpow(vec[i], tmp);                        \
    }

#define Array_set(vec, value) Array_set_filter(vec, true, value)
#define Array_add(vec, value) Array_add_filter(vec, true, value)
#define Array_sub(vec, value) Array_sub_filter(vec, true, value)
#define Array_mul(vec, value) Array_mul_filter(vec, true, value)
#define Array_div(vec, value) Array_div_filter(vec, true, value)
#define Array_mod(vec, value) Array_mod_filter(vec, true, value)
#define Array_pow(vec, value) Array_pow_filter(vec, true, value)

#define Array_set1(vec, value) Array_set1_filter(vec, true, value)
#define Array_add1(vec, value) Array_add1_filter(vec, true, value)
#define Array_sub1(vec, value) Array_sub1_filter(vec, true, value)
#define Array_mul1(vec, value) Array_mul1_filter(vec, true, value)
#define Array_div1(vec, value) Array_div1_filter(vec, true, value)
#define Array_mod1(vec, value) Array_mod1_filter(vec, true, value)
#define Array_pow1(vec, value) Array_pow1_filter(vec, true, value)

// Promoting elemwise ops

// vec[7:9] = arr2[6:8] + sin(arr2[4:6]) + 3 + count(vec[vec<5]) + M ** x

// 0. wrap into an outer scope
// if true:
//     vec[7:9] = arr2[6:8] + sin(arr2[4:6]) + 3 + _1 + M ** vec

// 1. promote filtered reductions as usual.
// if true:
//     _1 = count(vec[vec<5])
//     vec[7:9] = arr2[6:8] + sin(arr2[4:6]) + 3 + _1 + M ** vec

// 2. promote exprs that are NOT elemwise ops. e.g. matmuls
// those may result in arrays that will participate in the
// elemwise op.

// if true:
//     _1 = count(vec[vec<5])
//     _2 = M ** vec
//     vec[7:9] = arr2[6:8] + sin(arr2[4:6]) + 3 + _1 + _2[:]

// 3. create an index variable for each array site in the expr.
// the initial value of this is the start of the slice.

// if true:
//     _1 = count(vec[vec<5])
//     _2 = M ** vec
//     _i1 = 7
//     _i2 = 6
//     _i3 = 4
//     _i4 = 0
//     vec[7:9] = arr2[6:cx] + sin(arr2[4:cm]) + 3 + _1 + _2[:-1]

// 4. create a span value and delta value for each array site.
// if true:
//     _i1 = realind(vec,7)
//     _i2 = realind(arr2,6)
//     _i3 = realind(arr2,4)
//     _i4 = realind(_2,0)
//     _d1 =  1
//     _d2 =  1
//     _d3 =  1
//     _d4 =  1
//     _s1 = realind(vec, 9) - _i1
//     _s2 = realind(arr2,cx) - _i2
//     _s3 = realind(arr2,cm) - _i3
//     _s4 = realind(_2, -1) - _i4
//     _1 = count(vec[vec<5])
//     _2 = M ** vec
//     vec[7:9] = arr2[6:cx] + sin(arr2[4:cm]) + 3 + _1 + _2[:-1]

// 5. replace the range exprs in the elemwise op with the indices.
// if true:
//     _i1 = realind(vec,7)
//     _i2 = realind(arr2,6)
//     _i3 = realind(arr2,4)
//     _i4 = realind(_2,0)
//     _s1 = realind(vec, 9) - _i1
//     _s2 = realind(arr2,cx) - _i2
//     _s3 = realind(arr2,cm) - _i3
//     _s4 = realind(_2, -1) - _i4
//     _1 = count(vec[vec<5])
//     _2 = M ** vec
//     vec[_i1] = arr2[_i2] + sin(arr2[_i3]) + 3 + _1 + _2[_i4]

//  6. move the elemwise op and the generated vars (not the
//  indices) into a loop. add incrementers for all _i1, _i2, ...
// if true:
//     let _i1 = realind(vec,7)
//     let _i2 = realind(arr2,6)
//     let _i3 = realind(arr2,4)
//     let _i4 = realind(_2,0)
//     let _s1 = realind(vec, 9) - _i1
//     let _s2 = realind(arr2,cx) - _i2
//     let _s3 = realind(arr2,cm) - _i3
//     let _s4 = realind(_2, -1) - _i4
//     for _i = _i1 : _s1 : _d1
//         let _1 = count(vec[vec<5])
//         let _2 = M ** vec
//         vec[_i1] = arr2[_i2] + sin(arr2[_i3]) + 3 + _1 +
//         _2[_i4] _i1 += _d1 _i2 += _d2 _i3 += _d3 _i4 += _d4

// 6a. add assertions for size consistency
// assert(_s1==_s2)
// assert(_s2==_s3)
// assert(_s3==_s4)

// 7. where possible, remove redundant calls to realind
// if true:
//     let _i1 = 7
//     let _i2 = 6
//     let _i3 = 4
//     let _i4 = 1
//     let _s1 = 9 - _i1
//     let _s2 = realind(arr2,cx) - _i2
//     let _s3 = realind(arr2,cm) - _i3
//     let _s4 = realind(_2, -1) - _i4
//     for _i = _i1 : _s1 : _d1
//         let _1 = count(vec[vec<5])
//         let _2 = M ** vec
//         vec[_i1] = arr2[_i2] + sin(arr2[_i3]) + 3 + _1 +
//         _2[_i4] _i1 += _d1 _i2 += _d2 _i3 += _d3 _i4 += _d4
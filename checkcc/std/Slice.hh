#define mutop(op)                                                          \
    void operator op(const Slice<T>& rhs)                                  \
    {                                                                      \
        for (auto l = start, r = rhs.start; l < stop;                      \
             l += step, r += rhs.step)                                     \
            *l op* r;                                                      \
    }                                                                      \
    void operator op(const Selection<T>& rhs)                              \
    {                                                                      \
        Int i = 0;                                                         \
        for (auto l = start; l < stop; l += step, i++)                     \
            *l op* rhs[i];                                                 \
    }                                                                      \
    void operator op(const Filter<T>& rhs)                                 \
    {                                                                      \
        auto l = start;                                                    \
        for (Int i = 0; i < len(rhs.ref); i++)                             \
            if (rhs.filter[i]) {                                           \
                *l op* rhs.ref[i];                                         \
                l += step;                                                 \
            }                                                              \
    }                                                                      \
    void operator op(const Array<T>& rhs)                                  \
    {                                                                      \
        assert(len(this) == len(rhs));                                     \
        for (auto l = start, r = rhs.start; l < stop; l += step, r++)      \
            *l op* r;                                                      \
    }                                                                      \
    void operator op(const T& rhs)                                         \
    {                                                                      \
        for (auto l = start; l < stop; l += step)                          \
            *l op* rhs;                                                    \
    }

// Slice is a view of an array based on a (possibly strided) range.
template <class T, typename Base> struct Slice {
    Base<T>* ref; //, *stop;
    Int step;
    Slice<T>(const Base& arr, Int istart = 0, Int istop = 0, Int istep = 1)
    {
        start = istart + (istart < 0 ? arr.end : arr.start);
        stop = istop + (istop <= 0 ? arr.end : arr.start);
        step = istep;
    }
    T& operator[](Int idx) { return ref[idx * step]; }
    MUT_OPS
};

template <class T> static Int len(const Selection<T>& sel)
{
    return sel.count;
}

template <class T> static Int len(const Slice<T>& sli)
{
    return (sli.stop - sli.start) / step;
}

template <class T> static Array<T> toarray(const Slice<T>& sli)
{
    auto array = Array<T>(len(sli));
    for (auto x = sli.start; x < sli.stop; x += sli.step)
        pushf(array, *x);
    return array;
}
#define mutop(op)                                                          \
    void operator op(const Selection<T>& rhs)                              \
    {                                                                      \
        for (Int i = 0; i < count; i++)                                    \
            operator[](i) op* rhs[i];                                      \
    }                                                                      \
    void operator op(const Slice<T>& rhs)                                  \
    {                                                                      \
        for (Int i = 0; i < count; i++)                                    \
            operator[](i) op* rhs[i];                                      \
    }                                                                      \
    void operator op(const Array<T>& rhs)                                  \
    {                                                                      \
        for (Int i = 0; i < count; i++)                                    \
            operator[](i) op* rhs[i];                                      \
    }                                                                      \
    void operator op(const T& rhs)                                         \
    {                                                                      \
        for (Int i = 0; i < count; i++)                                    \
            operator[](i) op* rhs;                                         \
    }
// Selection is like a Slice, but based on a list of indexes.
template <class T, class Base> struct Selection {
    Base* ref;
    Int *indexes, count;
    Selection<T>(const Base& rref, Int* indexes, Int count)
    {
        ref = &rref;
        this->indexes = indexes;
        this->count = count;
    }
    T& operator[](Int idx) { return ref[indexes[idx]]; }
    // T& operator[](Int idx) { return start[indexes[idx]]; }
    MUT_OPS
};
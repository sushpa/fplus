
template <class T, int N> // N: no. of dimensions
struct SliceN {
    T *start, *stop;
    struct {
        Int istart, istop, step
    } dim[N];
};
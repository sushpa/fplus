
#define mutop(op)                                                          \
    void operator op(const NDArray<T, N>& rhs)                             \
    {                                                                      \
        for (auto l = start, r = rhs.start; l < stop; l++, r++)            \
            *l op* r;                                                      \
    }
// DECIDE what is 1-based and what is 0-based and where is the boundary
// in the API. For example best: index is 0-based, subscripts are 1-based
Int sub2idx(Int sub[], int dims) {}
void idx2sub(Int idx, Int sub[], int dims) {}

template <class T, int N> // N: no. of dimensions
struct NDArray : Vector<T> {
    Int dlen[N];
    T& operator[](Int sub[]) { return start[sub2idx(sub, N)]; }
    NDArray<T, N>(Int size[])
    {
        Int total = 1;
        for (int i = 0; i < N; i++) {
            dlen[i] = size[i];
            total *= size[i];
        }
        resize(total);
    }
    // this should be overridden for 2,3,4,5,6,7 N
    MUT_OPS;
};

static_assert(sizeof(NDArray<double, 3>) == 48, "");
static_assert(sizeof(NDArray<double, 4>) == 56, "");
static_assert(sizeof(NDArray<double, 6>) == 72, "");

using CharMatrix = NDArray<char, 2>;

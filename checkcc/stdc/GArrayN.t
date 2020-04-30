#if !defined(T)
#error "`T` not defined for GArrayN"
#endif

#if !defined(N)
#error "`N` not defined for GArrayN"
#endif

#if !defined(I)
#error "`I` not defined for GArrayN"
#endif
#define T double
#define N 3
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

#define GArrayN(T, N, I) JOIN4(GArrayN, T, N, I)

typedef struct GArrayN(T, N, I) GArrayN(T, N, I);
struct GArrayN(T, N, I)
{
    T* ref;
    uint(I) count, alloc; // alloc 0 means unowned
    // also knowing alloc is good for downward resize / subsequent regrow
    uint(I) dcount[N];
    struct {
        // NOPE they will be 16B inner
        union {
            struct {
                uint(I) start, stop, step;
            };
            struct {
                uint(I) * indexList;
                uint(I) indexCount;
            };
        };
        union {
        };
    } dim[N];
    // row-maj / col-maj, transposed, ...
};

// 1-way pools based on type (no freeing)
// rename this to node pool

#define alloc(Type) alloc_##Type
#define alloc_used(Type) alloc_used_##Type //[sizeof(Type)/8-1]
#define alloc_buf(Type) alloc_buf_##Type //[sizeof(Type)/8-1]
#define alloc_total(Type) alloc_total_##Type //[sizeof(Type)/8-1]
/*
typedef struct {uint64_t x[1];} _st_1x8B;
typedef struct {uint64_t x[2];} _st_2x8B;
typedef struct {uint64_t x[3];} _st_3x8B;
typedef struct {uint64_t x[4];} _st_4x8B;
typedef struct {uint64_t x[5];} _st_5x8B;
typedef struct {uint64_t x[6];} _st_6x8B;
typedef struct {uint64_t x[7];} _st_7x8B;
typedef struct {uint64_t x[8];} _st_8x8B;
typedef struct {uint64_t x[9];} _st_9x8B;
typedef struct {uint64_t x[10];} _st_10x8B;
typedef struct {uint64_t x[11];} _st_11x8B;
typedef struct {uint64_t x[12];} _st_12x8B;
typedef struct {uint64_t x[13];} _st_13x8B;
typedef struct {uint64_t x[14];} _st_14x8B;
typedef struct {uint64_t x[15];} _st_15x8B;
typedef struct {uint64_t x[16];} _st_16x8B;

static _st_1x8B* alloc_buf_1;
static _st_2x8B* alloc_buf_2;
static _st_3x8B* alloc_buf_3;
static _st_4x8B* alloc_buf_4;
static _st_5x8B* alloc_buf_5;
static _st_6x8B* alloc_buf_6;
static _st_7x8B* alloc_buf_7;
static _st_8x8B* alloc_buf_8;
static _st_9x8B* alloc_buf_9;
static _st_10x8B* alloc_buf_10;
static _st_11x8B* alloc_buf_11;
static _st_12x8B* alloc_buf_12;
static _st_13x8B* alloc_buf_13;
static _st_14x8B* alloc_buf_14;
static _st_15x8B* alloc_buf_15;
static _st_16x8B* alloc_buf_16;

static void* alloc_buf[16];
alloc_buf[0] = (void*) &alloc_buf_1;
alloc_buf[1] = (void*) &alloc_buf_2;
alloc_buf[2] = (void*) &alloc_buf_3;
alloc_buf[3] = (void*) &alloc_buf_4;
alloc_buf[4] = (void*) &alloc_buf_5;
alloc_buf[5] = (void*) &alloc_buf_6;
alloc_buf[6] = (void*) &alloc_buf_7;
alloc_buf[7] = (void*) &alloc_buf_8;
alloc_buf[8] = (void*) &alloc_buf_9;
alloc_buf[9] = (void*) &alloc_buf_10;
alloc_buf[10] = (void*) &alloc_buf_11;
alloc_buf[11] = (void*) &alloc_buf_12;
alloc_buf[12] = (void*) &alloc_buf_13;
alloc_buf[13] = (void*) &alloc_buf_14;
alloc_buf[14] = (void*) &alloc_buf_15;
alloc_buf[15] = (void*) &alloc_buf_16;
* /
// 16 size classes from 8B upwards in steps of 8B
static uint32_t alloc_used[16];
static uint32_t alloc_total[16];
*/
#define MAKE_ALLOCATOR(Type, elementsPerBlock)                             \
    static const uint32_t alloc_max_##Type = elementsPerBlock;             \
    static Type* alloc_buf(Type);                                         \
    static uint32_t alloc_used(Type) = alloc_max_##Type;                  \
    static uint32_t alloc_total(Type) = 0;                                \
                                                                           \
    Type* alloc(Type)()                                                   \
    {                                                                      \
        if (alloc_used(Type) >= elementsPerBlock) {                       \
            alloc_buf(Type) = (Type*) calloc(elementsPerBlock, sizeof(Type));     \
            alloc_used(Type) = 0;                                         \
        }                                                                  \
        alloc_total(Type)++;                                              \
        return &alloc_buf(Type)[alloc_used(Type)++];                     \
    }                                                                      \
                                                                           \
    void alloc_stat_##Type()                                               \
    {                                                                      \
        fprintf(stderr, "*** %u (%s) allocated (%ld B)\n",                 \
            alloc_total(Type), #Type, alloc_total(Type) * sizeof(Type)); \
    }

#define ELEMPERBLOCK 512

MAKE_ALLOCATOR(ASTExpr, ELEMPERBLOCK)

void alloc_stat() { alloc_stat_ASTExpr(); }
#undef join


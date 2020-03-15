// 1-way pools based on type (no freeing)
// rename this to node pool
#define MAKE_ALLOCATOR(Type, elementsPerBlock)                                 \
    static const uint32_t alloc_max_##Type = elementsPerBlock;                 \
    static Type* alloc_buf_##Type;                                             \
    static uint32_t alloc_used_##Type = alloc_max_##Type;                      \
    static uint32_t alloc_total_##Type = 0;                                    \
                                                                               \
    Type* alloc_##Type()                                                \
    {                                                                          \
        if (alloc_used_##Type >= alloc_max_##Type) {                           \
            alloc_buf_##Type = calloc(alloc_max_##Type, sizeof(Type));         \
            alloc_used_##Type = 0;                                             \
        }                                                                      \
        alloc_total_##Type++;                                                  \
        return &alloc_buf_##Type[alloc_used_##Type++];                         \
    }                                                                          \
                                                                               \
    void alloc_stat_##Type()                                     \
    {                                                                          \
        fprintf(stderr, "%u (%s) allocated (%ld B)\n", alloc_total_##Type,    \
            #Type, alloc_total_##Type * sizeof(Type));                  \
    }

MAKE_ALLOCATOR(node_t, 512)

void alloc_stat() { alloc_stat_node_t(); }

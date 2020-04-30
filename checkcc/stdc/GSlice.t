enum SliceMode { sliceByIndexes, sliceByRange, sliceByLogical };
enum SliceType { sliceOfArray, sliceOfSlice, sliceDirect };
struct GSlice(T, I)
{
    union {
        T* ref;
        GSlice(T, I) * sref;
        GArray(T, I) * aref;
    };
    uint(I) count; // alloc 0 means unowned
    // also knowing alloc is good for downward resize / subsequent regrow
    enum SliceMode mode;
    enum SliceType type;

    union {
        uint(I) step;
        uint(I) * indexList;
        bool* logicalList;
    } slice;
    // row-maj / col-maj, transposed, ...
};
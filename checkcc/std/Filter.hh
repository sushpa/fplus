// Filter is like a Selection, but based on a list of booleans.
// Don't conflate this with the _filter macros, which apply a Boolean
// filter for various inline operations on matching elements of arrays
template <class T, class Base> struct Filter {
    Base* ref;
    bool* filter; // count is same as ref's count
    Selection<T>(const Base& rref, bool* filter)
    {
        ref = &rref;
        this->filter = filter;
    }
    T& operator[](Int idx)
    {
        // FIXME
        Int accidx = 0; // accumulate the "real" index
        Int l = len(ref);
        for (Int i = 0; i < l; i++)
            if (filter[i]) accidx++;
        return ref[accidx];
    }
    // T& operator[](Int idx) { return start[indexes[idx]]; }
    MUT_OPS
};
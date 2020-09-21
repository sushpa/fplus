// For monolithic builds, include this file before main()

#define STATIC static

#include "basics/Number.c"
#include "basics/String.c"
#include "basics/YesOrNo.c"
#include "basics/BitVector.c"
#include "basics/Colour.c"
#include "basics/Currency.c"
#include "basics/DateTime.c"
#include "basics/DiskItem.c"
#include "basics/Duration.c"
#include "basics/Range.c"
#include "basics/Rational.c"
#include "basics/Regex.c"
#include "basics/Size.c"

#include "collections/Array.c"
#include "collections/ArrayND.c"
#include "collections/Dict.c"
#include "collections/Filter.c"
#include "collections/List.c"
#include "collections/Selection.c"
#include "collections/Sequence.c"
#include "collections/SequenceND.c"
#include "collections/Slice.c"
#include "collections/SliceND.c"

#include "runtime/Pool.c"

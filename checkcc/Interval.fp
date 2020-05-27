
type IntRange
    var lo = 0
    var hi = 0
    always lo <= hi
end type

products(a as IntRange, b as IntRange) :=
    [ a.lo*b.lo, a.lo*b.hi, a.hi*b.lo, a.hi*b.hi ]

IntRange(x as Real) :=
    IntRange(lo = x, hi = x)

add(a as IntRange, b as IntRange) :=
    IntRange(lo = a.lo + b.lo, hi = a.hi + b.hi)

sub(a as IntRange, b as IntRange) :=
    IntRange(lo = a.lo + b.lo, hi = a.hi + b.hi)

mul(a as IntRange, b as IntRange) :=
    IntRange(
        lo = min(products(a, b)),
        hi = max(products(a, b)))
    #CSE for products() which is pure
    # i guess if it is a #define, then C backend will apply
    # CSE on the muls

function div(a as IntRange, b as IntRange) result (ret as IntRange)
    requires not b contains 0
    ret = IntRange(lo = a.lo / b.lo, hi = a.hi / b.hi) # NO!!
end function

function log(a as RealRange) result (ret as RealRange)
    -- range literals are just like array literals. How to disambiguate?
    requires not a intersects [-inf:0]
    ret = RealRange(lo = log(a.lo), hi = log(a.hi))
end function

function recip(a as RealRange) result (ret as RealRange)
    requires not a contains 0
    ret = RealRange(lo = 1/a.hi, hi = 1/a.lo)
end function

function sin(a as RealRange) result (ret as RealRange)
    ret = RealRange(lo = msin(a.lo), hi = mcos(a.hi))
    ensures ret within [-1,1]
end function

flip(a as IntRange) :=
    IntRange(lo = -a.hi, hi = -a.lo)
 # * -1

# negate(a as IntRange, b as IntRange) := [
#     IntRange(lo = a.lo + b.lo, hi = a.hi + b.hi),
#     IntRange(lo = a.lo + b.lo, hi = a.hi + b.hi),
# ]
 # but this would ret = 2 intervals: [-1, 1] -> [-inf, -1], [1, +inf]
negate(a as IntRange) :=
    NotInterval(lo = a.lo, hi = a.hi)

unite(a as IntRange, b as IntRange) :=
    IntRange(lo = a.lo + b.lo, hi = a.hi + b.hi)

intersect(a as IntRange, b as IntRange) :=
    IntRange(lo = max(a.lo, b.lo), hi = min(a.hi, b.hi)) or nil # might violate invariant


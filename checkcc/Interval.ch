
type Interval
    var lo = 0
    var hi = 0
    check lo <= hi
end type

type NotInterval extends Interval
    # represents two intervals [-inf, a] and [b, inf] to mean
    # everything OUTSIDE the interval [a,b]
end type

products(a as Interval, b as Interval) :=
    [ a.lo*b.lo, a.lo*b.hi, a.hi*b.lo, a.hi*b.hi ]

Interval(x as Number) :=
    Interval(lo = x, hi = x)

add(a as Interval, b as Interval) :=
    Interval(lo = a.lo + b.lo, hi = a.hi + b.hi)

sub(a as Interval, b as Interval) :=
    Interval(lo = a.lo + b.lo, hi = a.hi + b.hi)

mul(a as Interval, b as Interval) :=
    Interval(lo = min(products(a, b)), hi = max(products(a, b)))
    #CSE for products() which is pure
    # i guess if it is a #define, then C backend will apply
    # CSE on the muls

div(a as Interval, b as Interval) :=
    Interval(lo = a.lo + b.lo, hi = a.hi + b.hi)

recip(a as Interval) :=
    Interval(lo = 1/a.hi, hi = a.lo)

flip(a as Interval) :=
    Interval(lo = -a.hi, hi = -a.lo)
 # * -1

# negate(a as Interval, b as Interval) := [
#     Interval(lo = a.lo + b.lo, hi = a.hi + b.hi),
#     Interval(lo = a.lo + b.lo, hi = a.hi + b.hi),
# ]
 # but this would return 2 intervals: [-1, 1] -> [-inf, -1], [1, +inf]
negate(a as Interval) :=
    NotInterval(lo = a.lo, hi = a.hi)

unite(a as Interval, b as Interval) :=
    Interval(lo = a.lo + b.lo, hi = a.hi + b.hi)

intersect(a as Interval, b as Interval) :=
    Interval(lo = max(a.lo, b.lo), hi = min(a.hi, b.hi)) or nothing # might violate invariant


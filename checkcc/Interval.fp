
type Range
    var lo = 0
    var hi = 0
    always lo <= hi
end type

products(a as Range, b as Range) :=
    [ a.lo*b.lo, a.lo*b.hi, a.hi*b.lo, a.hi*b.hi ]

Range(x as Real) :=
    Range(lo = x, hi = x)

add(a as Range, b as Range) :=
    Range(lo = a.lo + b.lo, hi = a.hi + b.hi)

sub(a as Range, b as Range) :=
    Range(lo = a.lo + b.lo, hi = a.hi + b.hi)

mul(a as Range, b as Range) :=
    Range(
        lo = min(products(a, b)),
        hi = max(products(a, b)))
    #CSE for products() which is pure
    # i guess if it is a #define, then C backend will apply
    # CSE on the muls

function div(a as Range, b as Range) result (ret as Range)
    requires not b contains 0
    ret = Range(lo = a.lo / b.lo, hi = a.hi / b.hi) # NO!!
end function

function log(a as Range) result (ret as Range)
    -- range literals are just like array literals. How to disambiguate?
    # requires not a intersects [-inf:0]
    requires a.lo > 0
    ret = Range(lo = log(a.lo), hi = log(a.hi))
end function

log(a[] as Range) := snap(Range(lo = log(a[:].lo), hi = log(a[:].hi)))

# function log(a[] as Range) result (ret[] as Range)
function log(inp as List of Range)
    result ans as List of Range?
    var symTable as Dict of String and List of ASTNode = {
        "map" = [ASTNode(kind = .tkOpColon)]
    }
    var symTable as Dict of String of ASTNode = {
        "map" = { .kind = .tkOpColon },
        "third" = { .kind = .tkKeywordIf },
    }
    var symTable as Dict of String and ASTNode = {
        "map" = ASTNode(kind = .tkOpColon),
        "third" = ASTNode(kind = .tkKeywordIf)
    }
    -- range literals are just like array literals. How to disambiguate?
    # resize(&ret, to = size(a))
    # requires a[0].lo > 0
    # requires not a intersects [-inf:0]
    # ret = snap(Range(lo = log(a[:].lo), hi = log(a[:].hi)))
    for i = 1 to size(inp) do push(&ans, item: log(inp[i]))
    ans = snap(ans)
#  as List@Range
end function


function recip(a as Range) result (ret as Range)
    requires not a contains 0
    ret = Range(lo = 1/a.hi, hi = 1/a.lo)
end function

function sin(a as Range) result (ret as Range)
    ret = Range(lo = msin(a.lo), hi = mcos(a.hi))
    ensures ret within [-1,1]
end function

flip(a as Range) := Range(lo = -a.hi, hi = -a.lo)
 # * -1

# negate(a as Range, b as Range) := [
#     Range(lo = a.lo + b.lo, hi = a.hi + b.hi),
#     Range(lo = a.lo + b.lo, hi = a.hi + b.hi),
# ]
 # but this would ret = 2 intervals: [-1, 1] -> [-inf, -1], [1, +inf]
negate(a as Range) :=
    NotInterval(lo = a.lo, hi = a.hi)

unite(a as Range, b as Range) :=
    Range(lo = a.lo + b.lo, hi = a.hi + b.hi)

intersect(a as Range, b as Range) :=
    Range(lo = max(a.lo, b.lo), hi = min(a.hi, b.hi)) or nil # might violate invariant


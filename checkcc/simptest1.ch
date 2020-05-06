declare type String
declare type Strings
# declare type Point

# declared funcs are basically implemented in C, they
# can be funcs or macros, doesn't matter here
declare function print(what as String)
declare function print(what as Scalar)
declare function describe(what as String)
declare function describe(what as Scalar)
declare function json(p as Point)
# declare function Point_new_()

# need an inheritance graph to avoid two types inheriting from each other or generally mutually recursive inheritance
# need a call graph to understand recursion patterns etc. and more imp. to avoid runaway recursion in the compiler when e.g. a constructor and a function depend on each other

# when do you run sempass on a type? when an instance of it is used in a func (vars or args)
# and on a func? whenever it is encountered.
# start processing at main, and see where you go.
# that means dead code will not be analyzed, but what the heck, not my problem.

type Other
    var m = 43
    # var po = Point()
end type
# - parse, resolve, distribute/check types.
# - resolve typespecs of (in order)
#   - type supers
#   - func args
#   - func rets
# - set func selectors
# - resolve funcs of
#   - func body stmts

type Point
    # var p = Other()
    var x = fxfunc(3) # fxfunc(p, p.po)
    var y = 69.6723
    var z = y + 5.6 * x
    # var o = Other()
    var cstr = "xyz"
end type

# fxfunc(x as Scalar) := x * 1.5

function fxfunc(x as Scalar) returns Scalar
return x * 1.5
end function
# point(m as Scalar) := m + 4

function Point(x as Scalar)
    let p = Point()
    # json(p)
    # p.x = x
    describe(x)
    return p #+ 5
end function

function main(args as Strings) returns Scalar
    let po = Point()
    let pcx = Point(78)
    json(po)
    funky()
    # print(pcx)
    # po = 0
    # var pm as Scalar
    # var nzz = zeros(450)
    # nzz[2:65] = 1
    # nzz[1:2:-1] = random()
    # nzz[nzz<4] = 4
    # describe(sum(nzz[nzz<5]))
    # nzz[:] = 34
    # describe(sum(nzz))

    # sum will be promoted out, but it has an elemental op INSIDE it.
    # so in addition to the isElemental flag you need a hasElemental
    # function that dives.
    # Like is promotion where the loop repeats to check for multiple
    # promotions in the same statement, the same should happen here,
    # but the current stmt should also be set back to the newly promoted
    # one so that itcan be searched for embedded elemental ops. the next
    # stmt will be the original, so elemental ops remaining will be treated
    # again -- make sure that after elemental promotions are done, the
    # flag is unset.

    # the problem is that the term inside nzz will be promoted even though
    # it has a dependency. hopefully no one writes code like this.
    # -- promoted within the enclosing 'for', not outside it
    # nzz[7:89] = nzz[1:82] + sum(nzz[7:47]+nzz[80:120]) * nzz[31:112]

    # var pox = ui.Window(400x300)
    # var pox = new(Point, frame = 400x300)
    # pox.x = 6
    # pox.cstr.jmp = "mereger"
    # json(po)
    # var postr = pox.json()
end function

function funky()
    joyce()
end function

function joyce()
    var name = "BhAbru"
    var nam2 = "Bhabru"
    var x = fxfunc(3 + 5 + 4)
    describe(x)
    describe(x+5)
    var y = 3-x * 2.5 / x
    # check 7 < 7-x < x+y
    # check x + fxfuNC(y) - fxfunc(x) >=  3*y + 5 + 2*x
    check name == naM2
end function

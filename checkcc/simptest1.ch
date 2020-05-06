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

type Point
    var x = 0
    var y = 69.67
    var z = 3.14159
    var cstr = "{0, -1, 0}"
end type

fxfunc(what as Scalar) := what * 1.5

# point(m as Scalar) := m + 4

function Point(x as Scalar)
    return Point()
end function

function main(args as Strings) returns Scalar
    # funky(4+2)
    var po = Point()
    var pcx = Point(7)
    json(po)
    json(pcx)
    # po = 0
    # var pm as Scalar
    # var pox = ui.Window(400x300)
    # var pox = new(Point, frame = 400x300)
    # pox.x = 6
    # pox.cstr.jmp = "mereger"
    # json(po)
    # var postr = pox.json()
end function

function funky(scalar1 as Scalar)
    joyce(2)
end function

function joyce(a as scAlar)
    var name = "BhAbru"
    var nam2 = "Bhabru"
    var x = 3 + 5 + 4
    describe(x+5)
    if x == 23
        let m = 32
    end if
    var y = 3-x * 2.5 / x
    # check 7 < 7-x < x+y
    check x + fxfuNC(y) - fxfunc(x) >=  3*y + 5 + 2*x
    check name == naM2
end function

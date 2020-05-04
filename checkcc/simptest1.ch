declare type String
declare type Strings

# declared funcs are basically implemented in C, they
# can be funcs or macros, doesn't matter here
declare function print(what as String)
declare function print(what as Scalar)
declare function describe(what as String)
declare function describe(what as Scalar)

fxfunc(what as Scalar) := what * 1.5

function main(args as String) returns Scalar
    funky(4+2)
end function

function funky(scalar1 as Tensor3D)
    joyce(2)
end function

function joyce(a as scalar)
    var name = "Bhabru"
    var name2 = "Bhabrus"
    var x = 3 + 5 + 4
    describe(x+5)
    var y = 3-x * 2.5 / x
    check x + fxfuNC(y) - fxfunc(x) >=  3*y + 5 + 2*x
    check name == naME2
end function

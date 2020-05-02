declare type String
declare type Strings

declare function print(what as String)
declare function print(what as Scalar)
declare function describe(what as String)
declare function describe(what as Scalar)

function fxfunc(what as Scalar) returns Scalar
    return what*1.5
end function

function main(a as Strings) returns Scalar
    let name as String = "Bhabru"
    describe(name)
    let x = 3 + 5 + 4
    # let arr as Scalar[] = [4, 5, 6, 7, 8]
    x = x + 2
    describe(x+5)
    # describe(count(arr, filter = arr<6))
    let y = 3-x * 2.5 / x
    describe(y*3-x+54/x*3.1415)
    check x + fxfunc(y) == 3 + 5 + 2*x
    check 0 < x < 1
    # check name == "Jiok"
end function

# function xyz(nam as String) returns Scalar
#     var arr[] as String = ["Nest", "Hoouj", "Goog"]
#     var arr as String[] = ["Nest", "Hoouj", "Goog"]

# end function

# how about a disp function that prints the name of the var and the var
# actually this is the describe function
# x <Scalar> = 8
# x ~Scalar = 8
# var john Scalar = 8
# x @Scalar = 8

# function yow(x <Scalar>, args <Strings>) <Scalar>

# end function

# function yow(x `Scalar`, args @Strings) @Scalar

# end function

# function yow(x Scalar, args Strings) Scalar

# end function
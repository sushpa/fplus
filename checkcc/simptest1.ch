declare type String
declare type Strings

declare function print(what as String)
declare function print(what as Scalar)

function main(a as Strings) returns Scalar
    let name as String = "Bhabru"
    print(name)
    let x = 3 + 5
    x = x + 2
    print(x)
    let y = 3-x * 2 ^ x
    print(y)
end function

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
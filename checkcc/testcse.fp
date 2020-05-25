# declare type Strings
declare function print(boom as String, wha as Number)

declare function sum(size as Number) result (x as Number)

type TestType
    var m = 43
    var g = 33
    always 0 <= g <= 1
end type

function main(args[] as String) as Number
    check len(args[]) > 1

    print("Hello")
    print("Hello", wha = 5)
    var m = 0
    m = 3 + 5

    var x = 1 + 3 + 5
    var d[] = [1, 2, 3, 8]
    var sd[] = [1, 2, 3, 4]
    var z = 3.0 + 5
    var b = sum(d[2:300000])
    var l = m + x + z + sum(d[2:300000])
    var cgg = 6 + sum(d[2:300000])
    describe(m)

    check 4 < out <= 6
end function

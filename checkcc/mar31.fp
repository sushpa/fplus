type Complex
    var re = 0
    var im = 0
end type

function Complex(re Number, im Number) returns Complex
    var c = Complex()
    c.re = re
    c.im = im
    return c
end function

# function Complex(re Number, im Number) returns (out Complex)
#     out = Complex()
#     out.re = re
#     out.im = im
# end function

Complex(re Number) := Complex(re, 0)

enum CollType
    none
    array
    list
    tensor2D
    tensor3D
    tensor4D
    orderedSet
    sortedSet
end enum

# main returns nothing, since the error handling mechanism will report
# any issues to the caller of main.
declare type Strings
function main(args[:] as String) result (ret Number)
    # constructor call must have all named args
    var c = Complex() #re = 3, im = 5)
    var d = [2, 3, 4, 5, 6, 7, 8]
    print(d[:])
    # describe(d)
    # print(args[9])
    # json(d)
    # d[:] += 6
    print(d[9]+6)
    json(c)
    var q = sum(d)
    # doarr()
end function

# disallow func name used as arg or ret name
function sum(x[:] as Real) result (s as Real)
    do elem = x[:]
        s += elem
    end do
end function

function doarr()
    var arr = [3, 4, 5; 6, 7, 8; 9, 10, 11]
    print(arr)
    describe(arr)
    describe(sum(arr))
    json(arr)
    arr[2,-1] = 0
    describe(sum(arr))
end function


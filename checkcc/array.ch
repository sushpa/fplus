declare function push()
declare function pop()
declare function concat()
declare function describe()
declare function print()
declare type Strs
# do declare type
declare type Any

# implement op_div, op_mul etc. for arrays of numeric types.
# BOTH inplace and non inplace versions!!
# function push(arr as Any, item as Any)
#     var count = arr.count or 0
#     count = count + 1
#     arr[count] = item
#     arr.count = count
# end function

# function pop(arr as Any)
#     var count = arr.count or 0
#     var ret = nil
#     if count > 0
#         count = count - 1
#         ret =  arr[count] 
#         arr[count] = nil
#     end if
#     return ret
# end function

function start(a as Strs)
    var arr as Scalar[] = [1, 2, 3, 4, 5] # literal ints hint at int unless a division or other op is found
    # generate the above = to a `Array_init_cArray` call
    push(arr, 3)
    let m = pop(arr)
    push(arr, 4)
    arr = 3 + 4
    push(arr, m)
    concat(arr, [5, 6, 7, 8]) # just a `Array_concat_cArray` call
    let arrd as Scalar[] = arr / 2 # asking for double
    describe(arr) # 4-element Array<Int>... [ 1, 2, 3, 5]
    print(arrd) # [1.0, 2.0, 3.0, 5.0]

    # var bgs as Dict
    # bgs["set"] = 5
    # bgs["get"] = 8
    # describe(bgs)
    # print(bgs)

end function


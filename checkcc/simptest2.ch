declare type Strs
declare type Range
declare function print(w as Scalar)
declare function print(w as Strs, filter as Logical)
declare function random(w as Range)

function start(args as Strs) returns Scalar
    var arr as Scalar[] = [1, 2, 3, 4, 5]
    print(arr[3])
    print(arr)
    arr += 3
    print(arr)
    var mx as Scalar[] = arr[1:3]
    # create lets by ref and vars by copy. SIMPLE!
    # random (int) takes a range;
    # float is always no args -> 0 to 1
    # internally rand float is obtained by getting a random int
    # over entire range and dividing it by max int

    if print(args, filter = 5) + random(1:2) == 1
        mx[mx<=2] = 12 # btw cgen will treat unchanged vars as const
    else
        if print(args, filter = 2) == 5
            mx = 5
        else
            mx = 3
        end if
    end if
    print(mx)
end function


function start(args as Strs) returns Scalar
    var strTable = [ "first" = 4, "second" = 5, "third" = 42 ]
    var list = [ 1, 2, 3, 4, 5; 6, 7, 8, 9, 0 ]
    strTable["fourth"] = 31337
    print(strTable)
    list[list%2==0] = 8
    print(list)
    # for var x = list[list%2==0] do x -= 1
 end function
function start(args as Strs)
    var x as Int = first(34)
    # print("Hello, World %d!\n", x)
end function

function first(a as Int) as Int
    var m as Int = second(5)
    return m
end function

function second(a as Int) as Int
    var m as Int = third(5)
    return m
end function

function third(a as Int) as Int
    var m as Int = first(5)
    return m
end function

function final(a as Int) as Int
    var m as Int = 3
    return m
end function
 
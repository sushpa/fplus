type DiskItem
    var fileName = "/"
    var fileSize = 0
    var fileMTime = DateTime()
    check(fileName != "")
end type

type DateTime
    base DiskItem
    var hour = 0
    var minute = 0
    var second = 0
    var day = 1
    var month = 1
    var year = 1970
    check(0 <= hour < 24)
    check(0 <= minute < 60)
    check(0 <= second < 60)
    check(1 <= day <= 31)
    check(1 <= month <= 12)
    check(1 <= year <= 5000)
end type

function ASTExpr.gen()
end function

function start(a as String[])
    var currentTime = DateTime()
    var currentFolder as String = os.pwd()
    var ss as Int = sys.stackSize()

    print(d.year)
    # if(d)
    # printf(d.year)...
    # else
    # nullptr exception
    # print(future(days = 3), past(years = 300), end = "")
    print(f.fileName, replace(f.fileName, what="guy", with="mex"))
    print(f.fileName, " in ", d.year)
    print(Timer.elapsed())
    print(timer.elapsed())
end function

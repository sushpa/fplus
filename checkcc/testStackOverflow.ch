# type MyBasicType
#     var fileName = ""
# #    var fileSize = 0
#     get fileSize => os.fsize(fileName) # short form
#     set filePerms(value) => os.chmod(fileName, value) # short form
    
#     # get fileSize # long form
#     #     return os.fsize(fileName)
#     # end get

#     # set fileETime(value) # long form
#     #     doSomething()
#     #     fgx(ff)
#     # end set

#     # get/set creates the variable automatically; no need to do var separately.
#     # in fact, doing it is an error (redefinition)
#     # declaring set auto declares a default get.
#     # declaring var auto declares both default set/get.

# end type

# type FolderItem
#     var fileName as String = "default name"
#     var fileSize as Int = 0
#     var nextItem as FolderItem = NULL
# end type



function start(a as Strs)
    var x as Int = NSDocument.initWithFrame(34 + 4 * 3)
    # var fi = FolderItem()
    # var fn as String = fi.fileName
    # print(fn)
    # print("Hello, World %d!\n", x)
end function

function NSDocument.initWithFrame(a as Int) as Int
    var m as Int = NSWindow.getFrame(5 - 4 + 2)
    return m
end function

function NSWindow.getFrame(a as Int) as Int
    var m as Int = HTTP.fromWeb(2 + 4 ^ 5)
    return m
end function

# function start(args as String)
    # var currTime as DateTime = repeat(DateTime(), times = 4)
    # var msx = [3, 4, 5, 6]
    # var currentFolder = split(os.pwd(), sep = ",")
    # var ss = duplicate(sys.stackSize(), times = 4)
# end function

function HTTP.fromWeb(a as Int) as Int
    var m as Int = NSDocument.initWithFrame(5 * 2.0)
    # var f as Range = m[1:3, g:sin(g)+x]
    return m
end function

function final(a as Int) as Int
    var m = 3
    # var cx = Strs.linesFromFile("~/basics.txt") 
    # if missing(cx) then break
    # print(myObjcount)
    return m
end function

# _____________________________________________________________________________
# readFileAndGetHeader: Do something and this is the documentation.
# filename: the initWithFrame argument.
# headerCols: the NSWindow.getFrame argument.
# function readFileAndGetHeader(filename, headerCols, options)
#     var mxp = 4 
#     var mg = mxp[map:3, :]
#     var bg = HTML()
#     check(0 < mg < 4)
#     mFileContents = readfile("wowee.txt") or return
#     mHeaders = trim(split(mFileContents[1]))
#     print("Wow $mg")
#     if mxp == 5
#         doSomethingElse()
#         for m in 1:45
#             mx = m + 4
#     readFileAndGetHeader = mg

# readFileAndGetHeader() = readFileAndGetHeader(0)

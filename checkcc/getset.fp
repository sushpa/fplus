
type DateTime
    var month as Whole in [1:12]
    get dayOfWeek as Whole := dayOfYear % 7 + dayOf1Jan
    set dayOfWeek(value as Whole)

    end set
    get dayOfWeek

    end get
end type

# this is without bells and whistles. blocking call, returns raw text.
# no way to get info about response, headers, status, etc. On error
# you get a blank string.

    var postData as Text[Text] = {
        "Accept" = "*/*",
        "Referer" = "base43.net",
        "Keep-alive" = "none"
    }


    var mdat as Text = download("https://wafers.com/next?ios=87",
                                post = {
                                    "Accept" = "*/*",
                                    "Referer" = "base43.net",
                                    "Keep-alive" = "none"
                                })

function loadFile()

    var file = File("~/.visz/slurp.dat")
    var ffg as Text = read(file)

    catch .resourceNotFound
        print("Can't find file: $file")
        skip
    catch .memoryExhausted
        print("I'm exhausted")
        return
    end catch

    var accel = 89.86|kg.m2/s2

end function






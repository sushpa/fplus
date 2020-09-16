

type DateTime
    var month as Int in [1:12]
    get dayOfWeek as Int := dayOfYear % 7 + dayOf1Jan
    set dayOfWeek(value as Int)

    end set
end type
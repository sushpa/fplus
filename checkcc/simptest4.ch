function start(args as Strs) returns Number
    let x = "The quick brown fox with 32 teeth."
    match x
    with `^Fat`
    else `teeth.$`
    else `quick (\w+) fox`
        print()
    end match
end function
#   For all i and j, d[i,j] will hold the Levenshtein distance between the     #
#   first "i" characters of "s" and the first "j" characters of "t".           #
#   Check details here: https://tinyurl.com/y857p2hf                       #
function levenstein(s as String, t as String)
    let m = len(s)
    let n = len(t)
    # let d as Int[:,:] = zeros2D([m, n])
    let d[m,n] = 0 #= zeros2D([m, n])

    d[1:m, 0] = [1:m]
    d[0, 1:n] = [1:n]

    do j = [1:n]
        do i = [1:m]
            let cost = num(s[i] != t[j])
            d[i, j] = min([ d[i-1, j  ] + 1,          # deletion
                            d[i,   j-1] + 1,          # insertion
                            d[i-1, j-1] + cost])      # substitution
        end do
    end do

    let dist = d[m,n]
    print("Distance between '$s' and '$t' is: $dist")
end function


function main(args[] as String) result (status as Number)
    levenstein("KITTEN", "SITTING")
    levenstein("JUMBO", "DUMBO")
    levenstein("NUMBER_OF_TIME_STEPS", "NUBER_OF_TIME_STEP")
    levenstein("PROBLEM_NAME", "PRUBLEM_NAME")
    check status in [0:255]
end function
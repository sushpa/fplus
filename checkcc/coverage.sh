#!/bin/sh
rm -r *.gcno *.gcda
make -B checkc-cov.bin # because it generates .gcno
# ./checkc-cov.bin simptest0.fp
for f in `find . -name '*.fp'`
do
    printf "\r$f" 1>&2
    ./checkc-cov.bin $f d > /dev/null 2>&1
    ./checkc-cov.bin $f l > /dev/null 2>&1
    ./checkc-cov.bin $f t > /dev/null 2>&1
done
echo "Unit                                        Unexec      Lines   Branches    Taken1+" > coverage.txt
echo "----                                        ------      -----   --------    -------" >> coverage.txt

gcov -f -b -a main.c | awk '

$1=="File" || $1=="Function" {
    printf "\n%s %-36s ",tolower(substr($1,1,4)), substr($2,2,length($2)-2)
}

END {
    printf "\n-- Executable lines = %.0f, not executed = %.0f --> %.1f%% coverage", totalLines, totalUnlines, (1-totalUnlines/totalLines)*100.0
}

$1=="Lines" || $1=="Branches" {
    var = substr($2,10)
    if ($1=="Lines") {
        lines = substr(var,1,length(var)-1)/100.0 * $4
        unlines = $4 - lines
        totalLines += $4*1
        totalUnlines+=unlines
        if (unlines > 0) {printf "%8.0f ", unlines}
            else { printf "%8s ", "" }
    }
    if (var*1.0 < 100.0) { printf "%9.0f%% ", var*1.0 }
    else { printf "%10s ", "" }
}

$1=="Taken" {
    taken = substr($4,6)*1.0
    if (taken < 100.0) printf "%9.0f%% ", taken
}

' | sort >> coverage.txt
less -S coverage.txt

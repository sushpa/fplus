#!/bin/sh
rm -r *.gcno *.gcda
make -B checkc-cov.bin # because it generates .gcno
# ./checkc-cov.bin simptest0.fp
for f in `find . -name '*.fp'`
do
    ./checkc-cov.bin $f > /dev/null 2>&1
done
echo "Unit                                           Lines   Branches    Taken1+" > coverage.txt
echo "----                                           -----   --------    -------" >> coverage.txt
gcov -f -b -a main.c | awk '
$1=="File" || $1=="Function" {
    printf "\n%s %-36s ",tolower(substr($1,1,4)), substr($2,2,length($2)-2)
}
$1=="Lines" || $1=="Branches" {
    printf "%10s ", substr($2,10)
}
$1=="Taken" {
    printf "%10s ", substr($4,6)
}' | sort >> coverage.txt
less -S coverage.txt

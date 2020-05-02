#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "Array.hh"

#define countof(x) (sizeof(x) / sizeof(x[0]))
int main()
{
    // Int mx[] = { 9, 8, 7, 6, 5, 4, 3, 2 };
    // auto arr = Vector<Int>(mx, countof(mx), true);
    // print(arr);
    // arr /= 3;
    // print(arr);
    // print(arr[3]);
    auto rnd = Vector<Int>(1UL << 48, true);
    auto rnd2 = Vector<Int>(rnd.alloc());
    rnd2 /= rnd;
    print(rnd2[0]);
}
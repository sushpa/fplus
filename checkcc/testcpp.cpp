#include <iostream>
#include <cstdint>

int main()
{
    int16_t tm4p = 0;
    int8_t i8 = 2;
    int16_t i16 = 2;
    int16_t tmp = 0;

    int8_t* pi8 = &i8;
    int16_t* pi16 = &i16;

    int32_t* pi32 = (int32_t*)&i16;
    // *pi32

    // i8 = 2;
    // i16 = 1;

    printf("%p %d\n", pi32, *pi32);
    printf("%p %d\n", pi16, *pi16);
    printf("%p %d\n", pi8, *pi8);

    // cout << (int)i8 << endl;
    // cout << i16 << endl;
}
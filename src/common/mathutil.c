#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mathutil.h"

int shr[257]; // ugly way to get precalced fast right shift for division with power of two numbers

int getRand(int from, int to)
{
    int rnd;
    if (from > to) return 0;
    if (from==to) return to;
    rnd = from + (rand() % (to - from));
    return rnd;
}

int getShr(int n)
{
    int b = -1;
    do{
        b++;
    }while((n>>=1)!=0);
    return b;
}

void initMathUtil()
{
    int i;
    for (i=1; i<=256; i++)
    {
        shr[i] = getShr(i);
    }
}

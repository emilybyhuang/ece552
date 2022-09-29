#include <stdio.h>

int  main (int argc, char *argv[])
{
    register int a = 10 + 9;
    register int b = 8 - 2;
    register int c = a + b;
 
    int e = 8 * 7;

    return 0;
}

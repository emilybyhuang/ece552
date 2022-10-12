#include <stdio.h>

int main(int argc, char *argv[])
{
    // store the variables to registers, so that we will not load value from stack
    register int r1 = 0;
    register int r2 = 0;
    register int r3 = 0;

    // doest not sotre counter i in order to have load and store from stack opertaions instrution for variety
    int i;
    for (i = 0; i < 100000; i++)
    {
        r1 = r2 + 3;
        r3 = 2;
        r3 = r1 + r2;
    }

    return 0;
}

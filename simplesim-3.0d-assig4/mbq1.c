#define ARRAY_SIZE 131072

int main() {
    int array[ARRAY_SIZE] = {0};
    int i, j;
    for (j = 0; j < ARRAY_SIZE; ++j) {
       // the divisor is the 'n' value (in the report)
       if (j % 25 == 0)
        array[j] = j;
    }
}


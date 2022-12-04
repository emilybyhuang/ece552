#define ARRAY_SIZE 131072
//this parameter can be tuned as 0 or 32
#define THRESHOLD 32
int main() {
    int array[ARRAY_SIZE] = {0};
    int i, j;
    for (j = 0; j < ARRAY_SIZE; ++j) {
       //make it steady
       if (j % 32 < THRESHOLD)
        array[j] = j;
       //make it unsteady 
       else {
       //generate irregular patterns
       	if (j % 2 == 0) 
		i = 1;
        else
		i = -1;
        array[j+i] = j;
      }
   }
}
//cache block size is 8, which can hold 2 integers
//threshold = 32: miss rate = 0.014,miss count = 16253
//threshold = 0: miss rate = 0.0624, miss count = 82508

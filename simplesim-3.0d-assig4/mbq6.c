#define ARRAY_SIZE 4096
//#define THRESHOLD 28
int main() {
    double array[ARRAY_SIZE] = {0};
    int i, j, k;
    	int index = 0;
for (k = 0; k < 1000; k++) {
    for (j = 1; j < ARRAY_SIZE; j+=16) {
       //make it NO_PRED
       //int quotient = j / 32;
      /* if (j % 32 < THRESHOLD) {
	       //irregular accessing pattern
          array[j+2] = j;
	  array[j] = j;
	  array[j+1] = j;
       }
       else {
          array[j] = j; 
       }*/

	for (i = 0; i < 16; i++) {
	if (i == 0) index = 0;
	else if (i < 13) {
		if (i % 2 == 0) index = -1;
		else index = 1;
/*	} else if (i == 13) { //13 14 15
		index = 12;
	   } else if (i == 14) {
	        index = 5;
           } else if (i == 15) {
		index = 7;
           } else {
		index = i;
           }
 	   */
	} else {
		index = -1;
	}
       	   array[j+index] = j;
	   //printf("index is %d\n", j+index);
	}

    }
}
}

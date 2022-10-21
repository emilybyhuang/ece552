#include <stdio.h>

int main(int argc, char *argv[])
{
    // store the variables to registers, so that we will not load value from stack
    register int r1 = 0;
    register int r2 = 0;
    register int r3 = 0;
    int i;
    for (i = 0; i < 100000; i++)
    {
        ++r1;

	if (r1 % 7 == 0){
		++r2;			
	}
    }
    r3 = r2 + r1;	
    return r3;
}
// The choice of the modulo divisor should be larger than 7.
// From the prelab, the instruction of the if block is 'i % 4', which needs 3 history bits to fully remember
// the pattern and consistently make correct predictions after a few training loops.
// Therefore, to correctly predict all branch outcomes, the modulo divisor should be no more than 
// (1 + # of branch history bits). 
//
// 2-level PAp branch predictor uses 6-bit wide history, it can track the pattern of cycle of at most 7.
// The result of the microbenchmark with divisor = 7 (A) has only 2486 mispredictions.
//
// The branch patterns for the microbenchmark with divisor = 10 (B & C) is NTTTTTTTTT (as one cycle), there will be 
// 1 misprediction per cycle at steady state. The misprediction will occur when predicting the next branch 
// outcome (which is N) at TTTTTT. The predictor will give a T instead of N, which is a wrong prediction. B (loop
// number 100k) has 12512 mispredictions and C (loop number 1M) has 102489 mispredictions. Subtracting the 
// mispredictions in A (2486), # of mispredictions in C is approximately 10 times # of mispredictions, which matches
// the 10 multiple of the number of loops in B and C.


// Excluding the misprediction during the training process
//
// A:
// Loop number = 100000, modulo divisor = 7:
// NUM_INSTRUCTIONS     	 :    2081127
// NUM_CONDITIONAL_BR   	 :     233243
// 2level:  NUM_MISPREDICTIONS   	 :       2486
// 2level:  MISPRED_PER_1K_INST  	 :      1.195

// B:
// Loop number = 100000, modulo divisor = 10:
// NUM_INSTRUCTIONS     	 :    2176925
// NUM_CONDITIONAL_BR   	 :     233240
// 2level:  NUM_MISPREDICTIONS   	 :      12512
// 2level:  MISPRED_PER_1K_INST  	 :      5.748
//
// C:
// Loop number = 1000000, modulo divisor = 10:
// NUM_INSTRUCTIONS     	 :   20266842
// NUM_CONDITIONAL_BR   	 :    2033243
// 2level:  NUM_MISPREDICTIONS   	 :     102489
// 2level:  MISPRED_PER_1K_INST  	 :      5.057


// 
/*
# mb.c:6:     register int r1 = 0;
	.loc 1 6 18
	movl	$0, %ebx	#, r1
# mb.c:7:     register int r2 = 0;
	.loc 1 7 18
	movl	$0, %r12d	#, r2
# mb.c:11:     for (i = 0; i < 100000; i++)
	.loc 1 11 12
	movl	$0, -28(%rbp)	#, i
# mb.c:11:     for (i = 0; i < 100000; i++)
	.loc 1 11 5
	jmp	.L2	#
.L4:
# mb.c:13:         ++r1;
	.loc 1 13 9
	addl	$1, %ebx	#, r1
# mb.c:15: 	if (r1 % 5 == 0){
	.loc 1 15 9
	movslq	%ebx, %rax	# r1, tmp88
	imulq	$1717986919, %rax, %rax	#, tmp88, tmp89
	shrq	$32, %rax	#, tmp90
	sarl	%eax	# tmp91
	movl	%ebx, %edx	# r1, tmp92
	sarl	$31, %edx	#, tmp92
	subl	%edx, %eax	# tmp92, _1
	movl	%eax, %edx	# _1, tmp93
	sall	$2, %edx	#, tmp93
	addl	%eax, %edx	# _1, tmp93
	movl	%ebx, %eax	# r1, _1
	subl	%edx, %eax	# tmp93, _1
# mb.c:15: 	if (r1 % 5 == 0){
	.loc 1 15 5
	testl	%eax, %eax	# _1
                                               // ----------MAIN BRANCH INSTRUCTION FOR ANALYSIS----------
	jne	.L3	#,                     // branch instruction of 'modulo if block'
                                               // NOT_TAKEN to enter the if statement
                                               // TAKEN to continue the next for loop
                                               
# mb.c:16: 		++r2;			
	.loc 1 16 3
	addl	$1, %r12d	#, r2
.L3:
# mb.c:11:     for (i = 0; i < 100000; i++)
	.loc 1 11 30 discriminator 2
	addl	$1, -28(%rbp)	#, i
.L2:
# mb.c:11:     for (i = 0; i < 100000; i++)
	.loc 1 11 5 discriminator 1
	cmpl	$99999, -28(%rbp)	#, i
	jle	.L4	#,                     // branch instruction of 'for loop' 

# mb.c:19:     r3 = r2 + r1;	
	.loc 1 19 8
	leal	(%r12,%rbx), %r13d	#, r3
# mb.c:20:     return r3;
	.loc 1 20 12
	movl	%r13d, %eax	# r3, _11
# mb.c:21: }
*/

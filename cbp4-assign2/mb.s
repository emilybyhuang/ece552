	.file	"mb.c"
# GNU C17 (Debian 10.2.1-6) version 10.2.1 20210110 (x86_64-linux-gnu)
#	compiled by GNU C version 10.2.1 20210110, GMP version 6.2.1, MPFR version 4.1.0, MPC version 1.2.0, isl version isl-0.23-GMP

# GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
# options passed:  -imultiarch x86_64-linux-gnu mb.c -mtune=generic
# -march=x86-64 -g -O0 -fverbose-asm -fasynchronous-unwind-tables
# options enabled:  -fPIC -fPIE -faggressive-loop-optimizations
# -fallocation-dce -fasynchronous-unwind-tables -fauto-inc-dec
# -fdelete-null-pointer-checks -fdwarf2-cfi-asm -fearly-inlining
# -feliminate-unused-debug-symbols -feliminate-unused-debug-types
# -ffp-int-builtin-inexact -ffunction-cse -fgcse-lm -fgnu-unique -fident
# -finline-atomics -fipa-stack-alignment -fira-hoist-pressure
# -fira-share-save-slots -fira-share-spill-slots -fivopts
# -fkeep-static-consts -fleading-underscore -flifetime-dse -fmath-errno
# -fmerge-debug-strings -fpeephole -fplt -fprefetch-loop-arrays
# -freg-struct-return -fsched-critical-path-heuristic
# -fsched-dep-count-heuristic -fsched-group-heuristic -fsched-interblock
# -fsched-last-insn-heuristic -fsched-rank-heuristic -fsched-spec
# -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-fusion
# -fsemantic-interposition -fshow-column -fshrink-wrap-separate
# -fsigned-zeros -fsplit-ivs-in-unroller -fssa-backprop -fstdarg-opt
# -fstrict-volatile-bitfields -fsync-libcalls -ftrapping-math -ftree-cselim
# -ftree-forwprop -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
# -ftree-loop-optimize -ftree-parallelize-loops= -ftree-phiprop
# -ftree-reassoc -ftree-scev-cprop -funit-at-a-time -funwind-tables
# -fverbose-asm -fzero-initialized-in-bss -m128bit-long-double -m64 -m80387
# -malign-stringops -mavx256-split-unaligned-load
# -mavx256-split-unaligned-store -mfancy-math-387 -mfp-ret-in-387 -mfxsr
# -mglibc -mieee-fp -mlong-double-80 -mmmx -mno-sse4 -mpush-args -mred-zone
# -msse -msse2 -mstv -mtls-direct-seg-refs -mvzeroupper

	.text
.Ltext0:
	.globl	main
	.type	main, @function
main:
.LFB0:
	.file 1 "mb.c"
	.loc 1 4 1
	.cfi_startproc
	pushq	%rbp	#
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp	#,
	.cfi_def_cfa_register 6
	pushq	%r13	#
	pushq	%r12	#
	pushq	%rbx	#
	.cfi_offset 13, -24
	.cfi_offset 12, -32
	.cfi_offset 3, -40
	movl	%edi, -44(%rbp)	# argc, argc
	movq	%rsi, -56(%rbp)	# argv, argv
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
	jne	.L3	#,
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
	jle	.L4	#,
# mb.c:19:     r3 = r2 + r1;	
	.loc 1 19 8
	leal	(%r12,%rbx), %r13d	#, r3
# mb.c:20:     return r3;
	.loc 1 20 12
	movl	%r13d, %eax	# r3, _11
# mb.c:21: }
	.loc 1 21 1
	popq	%rbx	#
	popq	%r12	#
	popq	%r13	#
	popq	%rbp	#
	.cfi_def_cfa 7, 8
	ret	
	.cfi_endproc
.LFE0:
	.size	main, .-main
.Letext0:
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.long	0xeb
	.value	0x4
	.long	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.long	.LASF10
	.byte	0xc
	.long	.LASF11
	.long	.LASF12
	.quad	.Ltext0
	.quad	.Letext0-.Ltext0
	.long	.Ldebug_line0
	.uleb128 0x2
	.byte	0x8
	.byte	0x7
	.long	.LASF0
	.uleb128 0x2
	.byte	0x4
	.byte	0x7
	.long	.LASF1
	.uleb128 0x2
	.byte	0x1
	.byte	0x8
	.long	.LASF2
	.uleb128 0x2
	.byte	0x2
	.byte	0x7
	.long	.LASF3
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.long	.LASF4
	.uleb128 0x2
	.byte	0x2
	.byte	0x5
	.long	.LASF5
	.uleb128 0x3
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x2
	.byte	0x8
	.byte	0x5
	.long	.LASF6
	.uleb128 0x4
	.byte	0x8
	.long	0x6b
	.uleb128 0x2
	.byte	0x1
	.byte	0x6
	.long	.LASF7
	.uleb128 0x5
	.long	.LASF13
	.byte	0x1
	.byte	0x3
	.byte	0x5
	.long	0x57
	.quad	.LFB0
	.quad	.LFE0-.LFB0
	.uleb128 0x1
	.byte	0x9c
	.long	0xe8
	.uleb128 0x6
	.long	.LASF8
	.byte	0x1
	.byte	0x3
	.byte	0xe
	.long	0x57
	.uleb128 0x2
	.byte	0x91
	.sleb128 -60
	.uleb128 0x6
	.long	.LASF9
	.byte	0x1
	.byte	0x3
	.byte	0x1a
	.long	0xe8
	.uleb128 0x3
	.byte	0x91
	.sleb128 -72
	.uleb128 0x7
	.string	"r1"
	.byte	0x1
	.byte	0x6
	.byte	0x12
	.long	0x57
	.uleb128 0x1
	.byte	0x53
	.uleb128 0x7
	.string	"r2"
	.byte	0x1
	.byte	0x7
	.byte	0x12
	.long	0x57
	.uleb128 0x1
	.byte	0x5c
	.uleb128 0x7
	.string	"r3"
	.byte	0x1
	.byte	0x8
	.byte	0x12
	.long	0x57
	.uleb128 0x1
	.byte	0x5d
	.uleb128 0x7
	.string	"i"
	.byte	0x1
	.byte	0xa
	.byte	0x9
	.long	0x57
	.uleb128 0x2
	.byte	0x91
	.sleb128 -44
	.byte	0
	.uleb128 0x4
	.byte	0x8
	.long	0x65
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x39
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_aranges,"",@progbits
	.long	0x2c
	.value	0x2
	.long	.Ldebug_info0
	.byte	0x8
	.byte	0
	.value	0
	.value	0
	.quad	.Ltext0
	.quad	.Letext0-.Ltext0
	.quad	0
	.quad	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF11:
	.string	"mb.c"
.LASF0:
	.string	"long unsigned int"
.LASF12:
	.string	"/homes/h/huangba9/ece552/cbp4-assign2"
.LASF2:
	.string	"unsigned char"
.LASF13:
	.string	"main"
.LASF10:
	.string	"GNU C17 10.2.1 20210110 -mtune=generic -march=x86-64 -g -O0 -fasynchronous-unwind-tables"
.LASF6:
	.string	"long int"
.LASF8:
	.string	"argc"
.LASF3:
	.string	"short unsigned int"
.LASF4:
	.string	"signed char"
.LASF5:
	.string	"short int"
.LASF1:
	.string	"unsigned int"
.LASF7:
	.string	"char"
.LASF9:
	.string	"argv"
	.ident	"GCC: (Debian 10.2.1-6) 10.2.1 20210110"
	.section	.note.GNU-stack,"",@progbits

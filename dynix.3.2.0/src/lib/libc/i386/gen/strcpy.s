/*
 * $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

	.file	"strcpy.s"
	.text
	.asciz	"$Header: strcpy.s 1.6 87/07/20 $"

/*
 * strcpy(s1, s2)
 *	char *s1, *s2;
 *
 * Copy string s2 to s1.  S1 must be large enough.
 * Return s1.
 */

#include "DEFS.h"

#ifdef	OLDWAY

#define	STRCPY \
	; slodb			/* 5, get byte, incr %esi */ 	\
	; sstob			/* 4, put byte, incr %edi */ 	\
	; testb	%al,%al		/* 2, end of string? */		\
	; jz	strcpy_done	/* 3 or 7+M */

ENTRY(strcpy)
	ENTER
	movl	%edi,%ecx	# save registers
	movl	%esi,%edx
	movl	FPARG0,%edi	# destination
	movl	FPARG1,%esi	# source

strcpy_loop:
	STRCPY			# 20
	STRCPY			# 20
	STRCPY			# 20
	STRCPY			# 20
	STRCPY			# 20
	STRCPY			# 20
	STRCPY			# 20
	STRCPY			# 20
	jmp	strcpy_loop	# 8

strcpy_done:
	movl	%edx,%esi	# restore registers
	movl	%ecx,%edi
	movl	FPARG0,%eax	# return value
	EXIT
	RETURN

#else	not OLDWAY

/*
 * Implement reading and writing words until a word
 * found with a null byte given by the following:
 *
 * has_null_byte(x) if [ (x - 0x01010101) & ~x & 0x80808080 ] != zero
 */

#define	STRCPY \
;	slodl			/*  5| get value in eax and incr source */ \
;	movl	%eax, %ecx	/*  2| copy value		        */ \
;	subl	%edx, %ecx	/*  2| (x - CONST1)		        */ \
;	notl	%eax		/*  2| ~x			        */ \
;	andl	%eax, %ecx	/*  2| (x - CONST1) & ~x		*/ \
;	notl	%eax		/*  2| x				*/ \
;	andl	%ebx, %ecx	/*  2| (x - CONST1) & ~x & CONST2       */ \
;	jnz	1f		/*  3| null byte detected?	        */ \
;	sstol			/*  4| no, so write 4 bytes and .. */

ENTRY(strcpy)
	pushl	%edi			# save 
	pushl	%esi			#   user
	pushl	%ebx			#     registers

	movl	16(%esp), %edi		# destination
	movl	20(%esp), %esi		# source
	movl	$0x80808080, %ebx	# constant one
	movl	$0x01010101, %edx	# constant two
0:
					# unroll loop for speed
	STRCPY
	STRCPY
	STRCPY
	STRCPY
	STRCPY
	jmp	0b			#  8| loop back
1:
	sstob				#  4| write byte
	testb	%al, %al		#  2| null byte?
	jz	2f			#  3| done if so
	movb	%ah, %al		#  2| get next byte
	sstob				#  4| write byte
	testb	%al, %al		#  2| null byte?
	jz	2f			#  3| done if so
	shrl	$16, %eax		#  3| get 2 bytes remaining
	sstob				#  4| write byte
	testb	%al, %al		#  2| null byte?
	jz	2f			#  3| done if so
	movb	%ah, (%edi)		#  2| write last byte and ...
2:
	popl	%ebx			# restore
	popl	%esi			#   user
	popl	%edi			#     registers
	movl	4(%esp), %eax		# load return value
	ret

#endif	not OLDWAY

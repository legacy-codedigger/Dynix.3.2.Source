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

	.file	"bcopy.s"
	.text
	.asciz	"$Header: bcopy.s 1.4 86/06/19 $"

/*
 * bcopy(source, destination, count)
 *	char *from, *to;
 *	unsigned int count;	(note "unsigned")
 *
 * Fast byte copy.  Moves double words then bytes
 * Checks for pointer equality (no work to do), Handles overlap correctly
 * Moving strings backward is twice as slow as forward so we are real
 * careful to only move backward if we *MUST*
 *
 * FOUR cases to contend with 
 * 1. S < D no overlap		3. S < D with overlap**
 * 2. D < S no overlap		4. D < S with overlap
 *
 * ** - MUST BE DONE IN REVERSE DIRECTION!
 */

#include "DEFS.h"

ENTRY(bcopy)
	ENTER
	movl	%edi,%eax	# save registers
	movl	%esi,%edx
	movl	FPARG0,%esi	# source
	movl	FPARG1,%edi	# destination
	movl	FPARG2,%ecx	# count
	testl	%ecx,%ecx	# any work to do?
	jz	bcopy_fini	# nope
	cmpl	%esi,%edi	# Source > Destination?
	jna	bcopy_smovb	# yes, so copy forward
	je	bcopy_fini	# equal? no copy
	addl	%ecx,%esi	# source += count
	cmpl	%esi,%edi	# overlap? (source+count > destination)?
	jna	bcopy_reverse	# yes, case 3
	movl	FPARG0,%esi	# no, so restore source
bcopy_smovb:
	rep;	smovb		# move the bytes
bcopy_fini:
	movl	%edx,%esi	# restore registers
	movl	%eax,%edi
	EXIT
	RETURN

bcopy_reverse:
	movl	FPARG0,%esi	# restore source
	addl	%ecx,%esi	# source += count - 1
	decl	%esi
	addl	%ecx,%edi	# destination += count - 1
	decl	%edi
	std			# set direction flag for backwards
	rep;	smovb		# move the bytes
	cld			# clear direction flag
	movl	%edx,%esi	# restore registers
	movl	%eax,%edi
	EXIT
	RETURN

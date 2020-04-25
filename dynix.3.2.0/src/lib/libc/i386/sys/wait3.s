/* $Copyright:	$
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

/* $Header: wait3.s 2.4 87/06/22 $
 *
 * $Log:	wait3.s,v $
 */

/*
 * C library -- wait3
 *
 * pid = wait3(&status, flags, &rusage);
 *
 * pid == -1 if error
 * status indicates fate of process, if given
 * flags may indicate process is not to hang or
 * that untraced stopped children are to be reported.
 * rusage optionally returns detailed resource usage information
 */
#include "SYS.h"

#define	SYS_wait3	SYS_wait

ENTRY(wait3)
	stc				# "Carry" == 1 ==> new flavor wait()
	movl	SPARG1, %ecx		# options
	movl	SPARG2, %edx		# &rusage
	SVC(2,wait3)			# do it.
	jc	err			# tsk, tsk...
	movl	SPARG0, %edx		# arg == pointer to status word.
	testl	%edx, %edx		# wants status?
	jz	done			# no -- skip it
	movl	%ecx, (%edx)		# yes -- save it
done:
	ret
CERROR

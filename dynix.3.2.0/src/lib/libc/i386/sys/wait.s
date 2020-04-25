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

/* $Header: wait.s 2.4 87/06/22 $
 *
 * $Log:	wait.s,v $
 */

#include "SYS.h"

ENTRY(wait)
	clc				# "carry" == 0 ==> old flavor wait()
	SVC(0,wait)
	jc	err			# tsk, tsk...
	movl	SPARG0, %edx		# arg == pointer to status word.
	testl	%edx, %edx		# wants status?
	jz	done			# no -- skip it
	movl	%ecx, (%edx)		# yes -- save it
done:
	ret
CERROR

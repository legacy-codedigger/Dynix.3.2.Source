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

/* $Header: mmap.s 2.5 87/06/22 $
 *
 * $Log:	mmap.s,v $
 */

#include "SYS.h"

ENTRY(mmap)
	SVC6(mmap)
	jc	err			# oops!
	movl	SPARG1, %edx		# 'len' argument
	addl	SPARG0, %edx		# addr+len
	cmpl	%edx, __curbrk		# did it grow?
	jge	ok			# nope.
	movl	%edx, __curbrk		# yes -- update the break.
ok:	ret
CERROR

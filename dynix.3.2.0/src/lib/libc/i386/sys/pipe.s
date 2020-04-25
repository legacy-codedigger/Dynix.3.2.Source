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

/* $Header: pipe.s 2.3 87/06/22 $
 *
 * $Log:	pipe.s,v $
 */

#include "SYS.h"

ENTRY(pipe)
	SVC(0,pipe)
	jc	err
	movl	SPARG0, %edx
	movl	%eax, (%edx)
	movl	%ecx, 4(%edx)
	xorl	%eax, %eax
	ret
CERROR

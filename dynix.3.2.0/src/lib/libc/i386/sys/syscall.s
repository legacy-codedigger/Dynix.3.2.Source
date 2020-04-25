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

/* $Header: syscall.s 2.5 87/06/23 $
 *
 * $Log:	syscall.s,v $
 */

#include "SYS.h"

ENTRY(syscall)
	leal	SPARG1, %ecx	# -> actual args.
	movl	$[SYS_REL<<16], %eax
	movw	SPARG0, %ax	# syscall number
	int	$BASE_SVC_INT+6	# copy up to 6 args.
	jc	err		# if error
	ret
CERROR

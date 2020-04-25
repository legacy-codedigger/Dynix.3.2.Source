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

/* $Header: exect.s 2.4 87/06/22 $
 *
 * This interface is obsolete given semantics of execve() of a traced
 * process -- execve() posts a SIGTRAP (psignal()) before the process
 * returns from exec.
 *
 * $Log:	exect.s,v $
 */

#include "SYS.h"

#define	FLAGS_TF	0x100		/* trace-trap bit */

ENTRY(exect)
	leal	SPARG0, %ecx	# -> args.
	movl	$[SYS_execve|[SYS_REL<<16]], %eax # sys call #
	pushl	$FLAGS_TF	# so can turn...
	popfl			#	...trace-flag.
	int	$BASE_SVC_INT+3	# 3-arg syscall.
CERROR				# exect(file, argv, env); any return is error

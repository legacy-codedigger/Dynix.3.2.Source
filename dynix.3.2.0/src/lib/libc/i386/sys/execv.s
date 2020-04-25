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

/* $Header: execv.s 2.5 87/06/22 $
 *
 * $Log:	execv.s,v $
 */

#include "SYS.h"

ENTRY(execv)
	pushl	_environ	# default to current environ.
	pushl	4+SPARG1	# argv
	pushl	8+SPARG0	# file
	call	_execve		# execv(file, argv)
	addl	$12, %esp	# clear stack (redundant since no saved regs)
	ret

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

/* $Header: execle.s 2.5 87/06/22 $
 *
 * $Log:	execle.s,v $
 */

#include "SYS.h"

ENTRY(execle)
	leal	SPARG1, %eax	# &arg1
looper:	cmpl	$0, (%eax)	# found end of args?
	je	found		# yup.
	addl	$4, %eax	# nope -- try next
	jmp	looper		#
found:	pushl	4(%eax)		# env
	leal	4+SPARG1, %eax	# &arg1
	pushl	%eax		#
	pushl	8+SPARG0	# file
	call	_execve		# execle(file, arg1, arg2, ... 0, env);
	addl	$12, %esp	# clear stack
	ret

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

/* $Header: shvfork.s 1.2 90/07/31 $
 */

/*
 * pid = shvfork(resource_mask)
 *
 * shared resource fork
 */

#include "SYS.h"
#include "dbsupport.h"

ENTRY(shvfork)
	popl	%edx		# pop return address
	pushl	$DB_SHVFORK	# 1st sys call arg
/*
 * Hack alert... shvfork is really a 2 argument system call.
 * Unfortunately, the 2 arg system call interface clobbers %edx
 * which must be preserved.  So we pretend its a 3 arg system call.
 */
	leal	(%esp), %ecx	# address of arguments
	SVC(3,dbsupport)	# 3 argument system call
	jc	err
	testl	%ecx, %ecx	# in parent, %ecx == 0
	je	done
	xorl	%eax, %eax	# child returns 0
	addl	$4, %esp
done:	jmp	*%edx

err:
	movl	%eax, _errno	# save in _errno
	movl	$-1, %eax	# return -1 to indicate error
	addl	$4, %esp
	jmp	*%edx

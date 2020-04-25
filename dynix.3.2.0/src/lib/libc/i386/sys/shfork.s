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

/* $Header: shfork.s 1.2 90/07/31 $
 */

/*
 * pid = shfork(resource_mask)
 *
 * shared resource fork
 */

#include "SYS.h"
#include "dbsupport.h"

ENTRY(shfork)
	movl	$DB_SHFORK, %ecx # arg 1
	movl	SPARG0, %edx	# arg 2
	SVC(2,dbsupport)	# 2 argument system call
	jc	err
	testl	%ecx, %ecx	# in parent, %ecx == 0
	je	done
	xorl	%eax, %eax	# child returns 0
done:	ret

CERROR

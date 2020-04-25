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

/* $Header: getmaxusers.c 1.1 88/05/20 $ */

/*
 * getmaxusers
 *	Return the max number of allowed users
 */
getmaxusers()
{
#if ns32000
	register int users;	/* known to be r7 */

	time(0);
	asm("movd	r1, r7");
	return (users);
#endif
#if i386
	register int users;	/* known to be %edi */

	time(0);
	asm("movl	%edx, %edi");
	return (users);
#endif
}

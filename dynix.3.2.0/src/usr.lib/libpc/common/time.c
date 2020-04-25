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

/* $Header: time.c 1.1 89/03/12 $ */
extern char *ctime();

TIME(alfap)
register char *alfap;
{
	register char *ap, *cp;
	register int i;
	static long a;

	time(&a);
	cp = ctime(&a);
	ap = alfap;
	for (cp = cp + 10, i = 10; i; *ap++ = *cp++, i--)
		/* void */;
}

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

/* $Header: bcopy.c 2.0 86/01/28 $ */

/* block copy from from to to, count bytes */
static char *sccsid = "@(#)bcopy.c	7.1	7/8/81";
bcopy(from, to, count)
#ifdef vax
	char *from, *to;
	int count;
{

	asm("	movc3	12(ap),*4(ap),*8(ap)");
}
#else
	register char *from, *to;
	register int count;
{
	while ((count--) > 0)	/* mjm */
		*to++ = *from++;
}
#endif

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

/* $Header: strcat.c 1.1 86/02/24 $
 *
 * Concatenate s2 on the end of s1.  S1's space must be large enough.
 * Return s1.
 */

char *
strcat(s1, s2)
register char *s1, *s2;
{
	register char *os1;

	os1 = s1;
	while (*s1++)
		;
	--s1;
	while (*s1++ = *s2++)
		;
	return(os1);
}

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

/* $Header: strncmp.c 1.1 86/03/05 $
 *
 * Compare strings (at most n bytes):  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */

strncmp(s1, s2, n)
register char *s1, *s2;
register n;
{

	while (--n >= 0 && *s1 == *s2++)
		if (*s1++ == '\0')
			return(0);
	return(n<0 ? 0 : *s1 - *--s2);
}

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

/* $Header: mktemp.c 2.0 86/01/28 $ */

char *
mktemp(as)
char *as;
{
	register char *s;
	register unsigned pid;
	register i;

	pid = getpid();
	s = as;
	while (*s++)
		;
	s--;
	while (*--s == 'X') {
		*s = (pid%10) + '0';
		pid /= 10;
	}
	s++;
	i = 'a';
	while (access(as, 0) != -1) {
		if (i=='z')
			return("/");
		*s = i++;
	}
	return(as);
}

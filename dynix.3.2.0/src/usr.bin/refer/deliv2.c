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

#ifndef lint
static char rcsid[] = "$Header: deliv2.c 2.0 86/01/28 $";
#endif

#include <stdio.h>

hash (s)
char *s;
{
	int c, n;
	for(n=0; c= *s; s++)
		n += (c*n+ c << (n%4));
	return(n>0 ? n : -n);
}

err (s, a)
char *s;
{
	fprintf(stderr, "Error: ");
	fprintf(stderr, s, a);
	putc('\n', stderr);
	exit(1);
}

prefix(t, s)
char *t, *s;
{
	int c;

	while ((c= *t++) == *s++)
		if (c==0) return(1);
	return(c==0 ? 1: 0);
}

char *
mindex(s, c)
char *s;
{
	register char *p;
	for( p=s; *p; p++)
		if (*p ==c)
			return(p);
	return(0);
}

zalloc(m,n)
{
	char *calloc();
	int t;
# if D1
	fprintf(stderr, "calling calloc for %d*%d bytes\n",m,n);
# endif
	t = (int) calloc(m,n);
# if D1
	fprintf(stderr, "calloc returned %o\n", t);
# endif
	return(t);
}

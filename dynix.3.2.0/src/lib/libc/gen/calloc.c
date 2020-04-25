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

/* $Header: calloc.c 2.1 86/04/01 $ */

/*
 * Calloc - allocate and clear memory block
 */
char *
calloc(num, size)
	register unsigned num, size;
{
	extern char *malloc();
	register char *p;

	size *= num;
	if (p = malloc(size))
		bzero(p, size);
	return (p);
}

cfree(p, num, size)
	char *p;
	unsigned num;
	unsigned size;
{
	free(p);
}

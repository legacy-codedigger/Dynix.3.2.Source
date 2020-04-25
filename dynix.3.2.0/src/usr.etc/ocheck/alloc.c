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

#ident	"$Header: alloc.c 1.2 90/02/12 $

/*
 * Common interface for allocating an array of pointers to elements, with
 * everything aligned on a page boundary
 */
#include <stdio.h>

extern char *valloc();

/* Local interface to valloc to exit on failure */
static char *
xalloc(size, name)
	int size;
	char *name;
{
	char *p = valloc(size);

	if (!p) {
		fprintf(stderr, "Can't allocate %s array\n", name);
		exit(1);
	}
	return(p);
}

/* Allocate an aligned array of elements */
char **
array_alloc(nptr, elemsize, name)
	int nptr, elemsize;
	char *name;
{
	register char **p;
	register int x;

	p = (char **)xalloc(nptr * sizeof(char *), name);
	p[0] = xalloc(nptr * elemsize, name);
	for (x = 1; x < nptr; ++x)
		p[x] = p[0] + (x * elemsize);
	return(p);
}

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

/* $Header: comp.c 2.0 86/01/28 $ */

#ifndef lint
static char sccsid[] = "@(#)comp.c	4.1 12/24/82";
#endif

#include <stdio.h>
#define MAX ' '

char new[MAX], old[MAX];

main ()
{
	register int i, j;
	old[0] = '\0';
	while (fgets(&new[0], MAX, stdin) != NULL) {
		for (i=0; i<MAX && old[i]==new[i]; i++);
		if (i >= MAX) {
			fprintf(stderr, "long word\n");
			exit(1);
		}
		putc(i, stdout);
		for (j=0; (old[j]=new[j]) != '\n'; j++);
		old[j] = '\0';
		fputs(&old[i], stdout);
	}
}

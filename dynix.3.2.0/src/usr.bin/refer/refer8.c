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
static char rcsid[] = "$Header: refer8.c 2.0 86/01/28 $";
#endif

#include "refer..c"

static char ahead[1024];
static int peeked = 0;
static char *noteof = (char *) 1;

char *
input(s)
char *s;
{
	if (peeked) {
		peeked = 0;
		if (noteof == 0)
			return(0);
		strcpy(s, ahead);
		return(s);
	}
	return(fgets(s, 1000, in));
}

char *
lookat()
{
	if (peeked)
		return(ahead);
	noteof = input(ahead);
	peeked = 1;
	return(noteof);
}

addch(s, c)
char *s;
{
	while (*s)
		s++;
	*s++ = c;
	*s = 0;
}

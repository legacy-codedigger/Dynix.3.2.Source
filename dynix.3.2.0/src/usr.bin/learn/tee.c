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
static char rcsid[] = "$Header: tee.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
main()
{
	int f, c;

	f = creat(".ocopy", 0666);
	while (read(0, &c, 1) == 1) {
		write (1, &c, 1);
		put(c, f);
	}
	fl(f);
	close(f);
}

static char ln[BUFSIZ];
char *p = ln;
put(c, f)
{
	*p++ = c;
	if (c == '\n') {
		fl(f);
		p=ln;
	}
}
fl(f)
{
	register char *s;

	s = ln;
	while (*s == '$' && *(s+1) == ' ')
		s += 2;
	write(f, s, p-s);
}

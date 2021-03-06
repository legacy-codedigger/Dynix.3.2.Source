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

#ident	"$Header: strtok.c 1.1 89/12/15 $"

/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strtok.c	5.4 (Berkeley) 6/27/88";
#endif /* LIBC_SCCS and not lint */

char *
strtok(s, sep)
	register char *s, *sep;
{
	register char *p;
	register c;
	static char *lasts;

	if (s == 0)
		s = lasts;
	if (s == 0)
		return (0);

	while (c = *s) {
		if (!index(sep, c))
			break;
		s++;
	}

	if (c == '\0') {
		lasts = 0;
		return (0);
	}

	for (p = s; c = *++p; )
		if (index(sep, c))
			break;

	if (c == '\0')
		lasts = 0;
	else {
		*p++ = '\0';
		lasts = p;
	}
	return (s);
}

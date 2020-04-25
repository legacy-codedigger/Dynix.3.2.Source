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

#if !defined(lint)
static char rcsid[] = "$Id: ato.c,v 1.1 88/09/02 11:47:54 ksb Exp $";
#endif lint

#include "whoami.h"
#include "0.h"

long
a8tol(cp)
	char *cp;
{
	int err;
	long l;
	register CHAR c;

	l = 0;
	err = 0;
	while ((c = *cp++) != '\0') {
		if (c == '8' || c == '9')
			if (err == 0) {
				error("8 or 9 in octal number");
				err++;
			}
		c -= '0';
		if ((l & 016000000000L) != 0)
			if (err == 0) {
				error("Number too large for this implementation");
				err++;
			}
		l = (l << 3) | c;
	}
	return (l);
}

/*
 * Note that the version of atof
 * used in this compiler does not
 * (sadly) complain when floating
 * point numbers are too large.
 */

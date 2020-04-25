/* @(#)$Copyright:	$
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

/* @(#)$Header: io.c 1.3 84/12/18 $ */

#include <stdio.h>
#include "host.h"

/*
 * g e t c h
 *
 * get a character from the port, return -1 for any read error
 */

int
getch()
{
	if (rcount > 0) {
		rcount--;
		return(*rptr++);
	}

	rptr = rbuf;
	rcount = read(port, rptr, BUFSIZ);
	if (rcount <= 0)
		return(-1);
	rcount--;
	return(*rptr++);
}

putch(c)
char c;
{
	write(port, &c, 1);
}

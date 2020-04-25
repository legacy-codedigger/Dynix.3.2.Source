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

#ifndef	lint
static char rcsid[] = "$Header: imsg.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)imsg.c	5.3 (Berkeley) 1/6/86";
#endif

#include "uucp.h"
#include <ctype.h>

char Msync[2] = "\020";

/* to talk to both eunice and x.25 without also screwing up tcp/ip
 * we must adaptively  choose what character to end the msg with
 * 
 * The idea is that initially we send ....\000\n
 * Then, after they have sent us a message, we use the first character
 * they send.
 */

int seenend = 0;
char Mend = '\0';

/*
 *	this is the initial read message routine -
 *	used before a protocol is agreed upon.
 *
 *	return codes:
 *		FAIL - no more messages
 *		SUCCESS - message returned
 */

imsg(amsg, fn)
char *amsg;
register int fn;
{
	register char *msg = amsg;
	register int nchars = 0;
	int foundsync = FAIL;
	char c;

	DEBUG(5, "imsg looking for SYNC<", CNULL);
	for (;;) {
		if (read(fn, &c, 1) != 1)
			return FAIL;
		c &= 0177;
		if (c == '\n' || c == '\r')
			DEBUG(5, "%c", c);
		else 
			DEBUG(5, (isprint(c) || isspace(c)) ? "%c" : "\\%o",
				c & 0377);
		if (c == Msync[0]) {
			DEBUG(5, ">\nimsg input<", CNULL);
			msg = amsg;
			nchars = 0;
			foundsync = SUCCESS;
			continue;
		} else if (foundsync != SUCCESS)
				continue;
		if (c == '\n' || c == '\0') {
			if (!seenend) {
				Mend = c;
				seenend++;
				DEBUG(9, "\nUsing \\%o as End of message char\n", Mend);
			}
			break;
		}
		*msg++ = c;
		/* MAXFULLNAME should really be passed in as a parameter */
		if (nchars++ > MAXFULLNAME) {
			DEBUG(1, "buffer overrun in imsg", CNULL);
			return FAIL;
		}
		fflush(stderr);
	}
	*msg = '\0';
	DEBUG(5, ">got %d characters\n", strlen(amsg));
	return foundsync;
}


/*
 *	this is the initial write message routine -
 *	used before a protocol is agreed upon.
 *
 *	return code:  always 0
 */

omsg(type, msg, fn)
register char *msg;
char type;
int fn;
{
	char buf[MAXFULLNAME];
	register char *c;

	c = buf;
	*c = '\0';	/* avoid pdp 11/23,40 auto-incr stack trap bug */
	*c++ = Msync[0];
	*c++ = type;
	while (*msg)
		*c++ = *msg++;
	*c++ = '\0';
	DEBUG(5, "omsg <%s>\n", buf);
	if (seenend) 
		c[-1] = Mend;
	else
		*c++ = '\n';
	write(fn, buf, (int)(c - buf));
	return 0;
}

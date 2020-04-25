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

/* $Header: psignal.c 2.0 86/01/28 $
 *
 * Print the name of the signal indicated
 * along with the supplied message.
 */
#include <signal.h>

extern	char *sys_siglist[];

psignal(sig, s)
	unsigned sig;
	char *s;
{
	register char *c;
	register n;

	c = "Unknown signal";
	if (sig < NSIG)
		c = sys_siglist[sig];
	n = strlen(s);
	if (n) {
		write(2, s, n);
		write(2, ": ", 2);
	}
	write(2, c, strlen(c));
	write(2, "\n", 1);
}

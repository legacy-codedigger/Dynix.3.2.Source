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
static char rcsid[] = "$Id: call.c,v 1.1 88/09/02 11:44:14 ksb Exp $";
#endif lint

/*
 * pxp - Pascal execution profiler
 *
 * Bill Joy UCB
 * Version 1.2 January 1979
 */

#include "0.h"
#include "tree.h"

/*
 * Procedure or function call
 */
call(p, argv)
	register int *argv;
{
	register *al;

	ppid(p);
	if (argv != NIL) {
		ppbra("(("+1);	/* xaproposstrange */
		for (;;) {
			al = argv[1];
			if (al[0] == T_WEXP) {
				rvalue(al[1], NIL);
				if (al[2] != NIL) {
					ppsep(": ");
					rvalue(al[2], NIL);
				}
				if (al[3] == OCT || al[3] == HEX) {
					ppspac();
					ppkw(al[3] == OCT ? "oct" : "hex");
				} else if (al[3] != NIL) {
					ppsep(": ");
					rvalue(al[3], NIL);
				}
			} else
				rvalue(argv[1], NIL);
			argv = argv[2];
			if (argv == NIL)
				break;
			ppsep(", ");
		}
		ppket(")");
	}
}

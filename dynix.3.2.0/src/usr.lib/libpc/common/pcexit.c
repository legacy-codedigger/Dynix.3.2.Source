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

/* $Header: pcexit.c 1.1 89/03/12 $ */
#include "h00vars.h"

PCEXIT(code)
int	code;
{
	static struct	{
		long	usr_time;
		long	sys_time;
		long	child_usr_time;
		long	child_sys_time;
	} tbuf;
	double l;

	PCLOSE(GLVL);
	PFLUSH();
	if (_stcnt > 0) {
		times(&tbuf);
		l = tbuf.usr_time;
		l = l / HZ;
		fprintf(stderr, "\n%1ld %s %04.2f seconds cpu time.\n",
				_stcnt, "statements executed in", l);
	}
	exit(code);
}

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

/* $Header: read8.c 1.1 89/03/12 $ */
#include "h00vars.h"
#include <errno.h>
extern int errno;

double
READ8(curfile)
register IOREC *curfile;
{
	static double data;
	register int retval;

	if (curfile->funit & FWRITE) {
		ERROR("%s: Attempt to read, but open for writing\n",
			curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	UNSYNC(curfile);
	errno = 0;
	retval = fscanf(curfile->fbuf, "%lf", &data);
	if (EOF == retval) {
		ERROR("%s: Tried to read past end of file\n", curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	if (0 == retval) {
		ERROR("%s: Bad data found on real read\n", curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	if (errno == ERANGE) {
		ERROR(0.0 == data ? "%s: Underflow on real read\n" :
		    "%s: Overflow on real read\n" , curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	if (0 != errno && errno != ENOTTY) {
		PERROR(curfile->pfname, curfile);
		/*NOTREACHED*/
		return;
	}
	curfile->funit &= ~EOLN;
	curfile->funit |= SYNC;
	return data;
}

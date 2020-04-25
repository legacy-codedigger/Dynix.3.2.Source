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

/* $Header: areaclear.c 2.0 86/01/28 $ */

#include "2648.h"

areaclear(rmin, cmin, rmax, cmax)
int rmin, cmin, rmax, cmax;
{
	int osm;
	char mes[20];
	register int i;

#ifdef TRACE
	if (trace)
		fprintf(trace, "areaclear(%d, %d, %d, %d)\n", rmin, cmin, rmax, cmax);
#endif
	osm = _supsmode;
	setclear();
	sync();
#ifdef notdef
	/* old kludge because I couldn't get area fill to work */
	for (i=rmax; i>=rmin; i--) {
		move(cmin, i);
		draw(cmax, i);
	}
#endif
	sprintf(mes, "%da1b%d %d %d %de", (_video==NORMAL) ? 1 : 2, cmin, rmin, cmax, rmax);
	escseq(ESCM);
	outstr(mes);
	_supsmode = osm;
}

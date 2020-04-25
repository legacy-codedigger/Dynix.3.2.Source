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

/* $Header: pmflush.c 1.1 89/03/12 $ */
#include "h00vars.h"

PMFLUSH(cntrs, rtns, bufaddr)
long cntrs;	/* total number of counters (stmt + routine) */
long rtns;	/* number of func and proc counters */
long *bufaddr;	/* address of count buffers */
{
	register FILE *filep;

	bufaddr[0] = 0426;
	time(&bufaddr[1]);
	bufaddr[2] = cntrs;
	bufaddr[3] = rtns;
	filep = fopen(PXPFILE, "w");
	if (filep == NULL)
		goto ioerr;
	fwrite(bufaddr, (int)(cntrs + 1), sizeof(long), filep);
	if (ferror(filep))
		goto ioerr;
	fclose(filep);
	if (!ferror(filep))
		return;
ioerr:
	perror(PXPFILE);
}

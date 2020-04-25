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

/* $Header: writes.c 1.1 89/03/12 $ */
#include "h00vars.h"

WRITES(curfile, d1, d2, d3, d4)
register IOREC	*curfile;
FILE *d1;
int d2, d3;
char *d4;
{
	if (curfile->funit & FREAD) {
		ERROR("%s: Attempt to write, but open for reading\n",
			curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	fwrite(d1, d2, d3, d4);
	if (ferror(curfile->fbuf)) {
		PERROR("Could not write to ", curfile->pfname);
		/*NOTREACHED*/
	}
}

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

/* $Header: seek.c 1.1 89/03/12 $ */
#include "h00vars.h"

/*
 * Random access routine
 */
SEEK(curfile, loc)
register IOREC *curfile;
struct seekptr *loc;
{
	curfile->funit |= SYNC;
	curfile->funit &= ~(EOFF | EOLN | SPEOLN);
	if (fseek(curfile->fbuf, loc->cnt, 0) == -1) {
		PERROR("Could not seek ", curfile->pfname);
		/*NOTREACHED*/
		return;
	}
}

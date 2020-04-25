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

/* $Header: readc.c 1.1 89/03/12 $ */
#include "h00vars.h"

char
READC(curfile)
register IOREC *curfile;
{
	if (curfile->funit & FWRITE) {
		ERROR("%s: Attempt to read, but open for writing\n", curfile->pfname);
		/*NOTREACHED*/
		return EOF;
	}
	IOSYNC(curfile);
	if (curfile->funit & EOFF) {
		ERROR("%s: Tried to read past end of file\n", curfile->pfname);
		/*NOTREACHED*/
		return EOF;
	}
	curfile->funit |= SYNC;
	return *curfile->fileptr;
}

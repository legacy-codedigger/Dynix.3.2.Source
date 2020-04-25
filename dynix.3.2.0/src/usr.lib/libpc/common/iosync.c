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

/* $Header: iosync.c 1.1 89/03/12 $ */
#include "h00vars.h"

/*
 * insure that a usable image is in the buffer window
 */
IOSYNC(curfile)
register IOREC	*curfile;
{
	register char *limit, *ptr;

	if (curfile->funit & FWRITE) {
		ERROR("%s: Attempt to read, but open for writing\n",
			curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	if ((curfile->funit & SYNC) == 0) {
		return;
	}
	if (curfile->funit & EOFF) {
		ERROR("%s: Tried to read past end of file\n", curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	curfile->funit &= ~SYNC;
	if (curfile->funit & SPEOLN) {
		curfile->funit &= ~(SPEOLN|EOLN);
		curfile->funit |= EOFF;
		return;
	}
	fread(curfile->fileptr, (int)curfile->fsize, 1, curfile->fbuf);
	if (ferror(curfile->fbuf)) {
		ERROR("%s: Tried to read past end of file\n", curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	if (feof(curfile->fbuf)) {
		if (curfile->funit & FTEXT) {
			*curfile->fileptr = ' ';
			if (curfile->funit & EOLN) {
				curfile->funit &= ~EOLN;
				curfile->funit |= EOFF;
				return;
			}
			curfile->funit |= (SPEOLN|EOLN);
			return;
		}
		curfile->funit |= EOFF;
		limit = &curfile->fileptr[curfile->fsize];
		for (ptr = curfile->fileptr; ptr < limit; )
			*ptr++ = 0;
		return;
	}
	if (curfile->funit & FTEXT) {
		if (*curfile->fileptr == '\n') {
			curfile->funit |= EOLN;
			*curfile->fileptr = ' ';
			return;
		}
		curfile->funit &= ~EOLN;
	}
}

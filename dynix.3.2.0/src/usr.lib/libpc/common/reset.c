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

/* $Header: reset.c 1.1 89/03/12 $ */
#include "h00vars.h"

RESET(filep, name, maxnamlen, datasize)
register IOREC *filep;
char *name;
long maxnamlen;
long datasize;
{
	if (name == NULL && filep == INPUT && filep->fname[0] == '\0') {
		if (fseek(filep->fbuf, (long)0, 0)) {
			PERROR("Could not reset ", filep->pfname);
			/*NOTREACHED*/
			return;
		}
		filep->funit &= ~EOFF;
		filep->funit |= (SYNC | EOLN);
		return;
	}
	filep = GETNAME(filep, name, maxnamlen, datasize);
	filep->fbuf = fopen(filep->fname, "r");
	if (filep->fbuf == NULL) {
		/*
		 * This allows unnamed temp files to be opened even if
		 * they have not been rewritten yet. We decided to remove
		 * this feature since the standard requires that files be
		 * defined before being reset.
		 */
#ifdef undef
		if (filep->funit & TEMP) {
			filep->funit |= (EOFF | SYNC | FREAD);
			/*NOTREACHED*/
			return;
		}
#else
		PERROR("Could not open ", filep->pfname);
		/*NOTREACHED*/
		return;
#endif
	}
	filep->funit |= (SYNC | FREAD);
	if (filep->funit & FTEXT)
		filep->funit |= EOLN;
	if (filep->fblk > PREDEF) {
		setbuf(filep->fbuf, &filep->buf[0]);
	}
}

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

/* $Header: rewrite.c 1.1 89/03/12 $ */
#include "h00vars.h"

REWRITE(filep, name, maxnamlen, datasize)
register IOREC *filep;
char *name;
long maxnamlen;
long datasize;
{
	filep = GETNAME(filep, name, maxnamlen, datasize);
	filep->fbuf = fopen(filep->fname, "w");
	if (filep->fbuf == NULL) {
		PERROR("Could not create ",filep->pfname);
		/*NOTREACHED*/
		return;
	}
	filep->funit |= (EOFF | FWRITE);
	if (filep->fblk > PREDEF) {
		setbuf(filep->fbuf, &filep->buf[0]);
	}
}

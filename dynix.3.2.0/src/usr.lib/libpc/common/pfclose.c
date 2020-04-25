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

/* $Header: pfclose.c 1.1 89/03/12 $ */
/*
 * Close a Pascal file deallocating resources as appropriate.
 */

#include "h00vars.h"
#include "libpc.h"

IOREC *
PFCLOSE(filep, lastuse)
register IOREC *filep;
bool lastuse;
{
	if ((filep->funit & FDEF) == 0 && filep->fbuf != NULL) {
		/*
		 * Have a previous buffer, close associated file.
		 */
		if (filep->fblk > PREDEF) {
			fflush(filep->fbuf);
			setbuf(filep->fbuf, NULL);
		}
		fclose(filep->fbuf);
		if (ferror(filep->fbuf)) {
			ERROR("%s: Close failed\n", filep->pfname);
			/*NOTREACHED*/
			return;
		}
		/*
		 * Temporary files are discarded.
		 */
		if ((filep->funit & TEMP) != 0 && lastuse &&
		    unlink(filep->pfname)) {
			PERROR("Could not remove ", filep->pfname);
			/*NOTREACHED*/
			return;
		}
	}
	_actfile[filep->fblk] = FILNIL;
	return filep->fchain;
}

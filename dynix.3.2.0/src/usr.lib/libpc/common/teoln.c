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

/* $Header: teoln.c 1.1 89/03/12 $ */
#include "h00vars.h"

bool
TEOLN(filep)
register IOREC *filep;
{
	if (filep->fblk >= MAXFILES || _actfile[filep->fblk] != filep ||
	    (filep->funit & FDEF)) {
		ERROR("Reference to an inactive file\n", 0);
		/*NOTREACHED*/
		return;
	}
	if (filep->funit & FWRITE) {
		ERROR("%s: eoln is undefined on files open for writing\n",
		    filep->pfname);
		/*NOTREACHED*/
		return;
	}
	IOSYNC(filep);
	if (filep->funit & EOFF) {
		ERROR("%s: eoln is undefined when eof is true\n",
		    filep->pfname);
		/*NOTREACHED*/
		return;
	}
	return filep->funit & EOLN ? TRUE : FALSE;
}

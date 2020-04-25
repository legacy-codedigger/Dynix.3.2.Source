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

/* $Header: pflush.c 1.1 89/03/12 $ */
#include "h00vars.h"

/*
 * insure that a usable image is in the buffer window
 */
PFLUSH()
{
	register IOREC *next;

	for (next = _fchain.fchain; next != FILNIL; next = next->fchain) {
		if ((next->funit & (FDEF | FREAD)) != 0)
			continue;
		if (next->fbuf != 0)
			fflush(next->fbuf);
	}
}

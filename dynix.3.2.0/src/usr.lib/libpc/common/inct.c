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

/* $Header: inct.c 1.1 89/03/12 $ */
#include "h00vars.h"

bool
INCT(element, paircnt, singcnt, data)
register long element;	/* element to find */
long paircnt;		/* number of pairs to check */
long singcnt;		/* number of singles to check */
long data;		/* paircnt plus singcnt bounds */
{
	register long *dataptr = &data;
	register int cnt;

	for (cnt = 0; cnt < paircnt; cnt++) {
		if (element > *dataptr++) {
			dataptr++;
			continue;
		}
		if (element >= *dataptr++) {
			return TRUE;
		}
	}
	for (cnt = 0; cnt < singcnt; cnt++) {
		if (element == *dataptr++) {
			return TRUE;
		}
	}
	return FALSE;
}

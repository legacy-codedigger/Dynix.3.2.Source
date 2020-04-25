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

/* $Header: max.c 1.1 89/03/12 $ */
#include "h00vars.h"

long
MAX(width, reduce, min)
register long width;		/* requested width */
long reduce;			/* amount of extra space required */
long min;			/* minimum amount of space needed */
{
	if (width <= 0) {
		ERROR("Non-positive format width: %D\n", width);
		/*NOTREACHED*/
	}
	return (width -= reduce) >= min ? width : min;
}

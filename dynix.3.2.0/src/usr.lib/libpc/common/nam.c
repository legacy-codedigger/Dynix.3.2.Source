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

/* $Header: nam.c 1.1 89/03/12 $ */
#include "h00vars.h"

char *
NAM(val, name)
long val;			/* internal enumerated type value */
char *name;			/* ptr to enumerated type name descriptor */
{
	register int	value = val;
	register short	*sptr;

	sptr = (short *)name;
	if (value < 0 || value >= *sptr) {
		ERROR("Enumerated type value of %D is out of range on output\n", val);
		/*NOTREACHED*/
		return;
	}
	sptr++;
	return	name+2+sptr[value];
}

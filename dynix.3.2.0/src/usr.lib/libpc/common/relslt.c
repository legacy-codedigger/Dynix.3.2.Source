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

/* $Header: relslt.c 1.1 89/03/12 $ */
#include "h00vars.h"

bool
RELSLT(size, str1, str2)
register long	size;
register char	*str1;
register char	*str2;
{
	while (*str1++ == *str2++ && --size)
		/* void */;
	return (size == 0) || (*--str1 >= *--str2) ? FALSE : TRUE;
}

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

#ifndef	lint
static	char	rcsid[] = "$Header: inval_cache.c 1.3 90/10/18 $";
#endif

/*
 * inval_cache.c
 *	Invalidate sgs2 processor cache
 *
 * This functionality used to be inline in startup.c.  Since the dynix3
 * assembler's optimizer insists on removing the .byte data from the text,
 * we need to have a separate source file that is compiled with no optimization.
 * This call should be put back in-line if the optimizer is ever fixed.
 * This call is made in the early part of the system's life, so performance
 * degradation is not an issue.
 */

/* $Log:	inval_cache.c,v $
 */

#include "../machine/mftpr.h"

inval_cache()
{
	INVAL_ONCHIP_CACHE();
}

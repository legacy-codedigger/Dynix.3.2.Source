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

#ifdef RCS
static char rcsid[]= "$Header: conf_xt.c 2.0 86/01/28 $";
#endif

/*
 * Binary configuration for standalone Xylogics 472 tape controller.
 */

#include "mbad.h"

struct	xtdevice *xtaddrs[] = {		/* controller addresses */
	(struct xtdevice *) (MB_IOSPACE+0x300),
};

#define	XTCTLRS	(sizeof(xtaddrs) / sizeof(xtaddrs[0]))

int	xtctlrs		= XTCTLRS;

/*
 * Set "xtdensel" non-zero *only* if the attached
 * tape drive accepts density selection.
 */

int	xtdensel	= 0;
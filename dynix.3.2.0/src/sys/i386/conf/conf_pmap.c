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
static	char	rcsid[] = "$Header: conf_pmap.c 2.3 87/02/13 $";
#endif

/*
 * conf_pmap.c
 *	Configuration for phys-mapped memory driver.
 */

/* $Log:	conf_pmap.c,v $
 */

#include "../h/param.h"

#include "../machine/hwparam.h"

#include "../balance/pmap.h"

/*
 * Initialize units:
 *	0-15 map 16 4k pieces of ALM space on MBAd[0].
 *	16-95 are reserved (can be run-time set by ioctl or changed here).
 *
 * RESERVE:	reserves a slot in the table; must be run-time filled out with
 *		PMAPIOCSETP to be useful.
 * PHYS:	set up a phys-mapped address range.  System services are not
 *		allowed where this is mapped.  Use for things that can't
 *		behave as generally as a memory board (eg, MBAd's).
 * NPMEM:	set up a non-paged memory address range.  System services
 *		are allowed where this is mapped.  Use for things like
 *		special purpose reserved memory (eg, for an accelerator).
 */

#define	ALM(m,i)	(PA_MBAd(m)+0xd0000+(i)*CLBYTES) /* ALM[i] on MBAd[m] */
#define	RESERVE		{ 0 }				/* reserve space */
#define	PHYS(a,s)	{ a, (s)/NBPG, 0 }		/* phys-map */
#define	NPMEM(a,s)	{ a, (s)/NBPG, PMAP_NPMEM }	/* non-paged mem */

struct	pmap_unit pmap_unit[96] = {
	PHYS( ALM(0,0), CLBYTES ),			/* 0 */
	PHYS( ALM(0,1), CLBYTES ),			/* 1 */
	PHYS( ALM(0,2), CLBYTES ),			/* 2 */
	PHYS( ALM(0,3), CLBYTES ),			/* 3 */
	PHYS( ALM(0,4), CLBYTES ),			/* 4 */
	PHYS( ALM(0,5), CLBYTES ),			/* 5 */
	PHYS( ALM(0,6), CLBYTES ),			/* 6 */
	PHYS( ALM(0,7), CLBYTES ),			/* 7 */
	PHYS( ALM(0,8), CLBYTES ),			/* 8 */
	PHYS( ALM(0,9), CLBYTES ),			/* 9 */
	PHYS( ALM(0,10), CLBYTES ),			/* 10 */
	PHYS( ALM(0,11), CLBYTES ),			/* 11 */
	PHYS( ALM(0,12), CLBYTES ),			/* 12 */
	PHYS( ALM(0,13), CLBYTES ),			/* 13 */
	PHYS( ALM(0,14), CLBYTES ),			/* 14 */
	PHYS( ALM(0,15), CLBYTES ),			/* 15 */
	/*
	 * Add entries 16 and up here.  If desire more than 96 entries,
	 * change the pmap_unit pmap_unit[xx] declaration, above.
	 */
};

int	pmap_nunit = sizeof(pmap_unit)/(sizeof(pmap_unit[0]));

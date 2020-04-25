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
static	char	rcsid[] = "$Header: conf_lp.c 1.4 87/01/30 $";
#endif

/*
 * conf_lp.c
 *	Systech line printer binary configuration file.
 */

/* $Log:	conf_lp.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/systm.h"
#include "../machine/ioconf.h"
#include "../mbad/mbad.h"
#include "../mbad/lp.h"	

/*
 * Parallel line printer configuration.
 *
 * When configuring a printer, it is best to insure
 * that the printer has passed the self-test procedure described
 * in the hardware reference manual for the mlp-2000 controller.
 * If the printer does not pass the self-test, check to see if
 * the cable to the printer is properly configured according to the
 * controller manual (this may save a service call). In particular
 * pins 32-34 of the cable should not be connected if the printer
 * is using a Centronics interface.
 *
 * For details on configuration, see lp(4).  The special_map field is
 * defined to contain one of:
 *
 * LPDEFAULT: 		Align the form on device open and close and
 *			interpret newlines, backspaces, tabs and form feeds.
 * LPCAPS:		Upper case only mode - printer assumed to have a
 *			small character set.
 * LPRAW:		Pass all 8 bits to the printer without interpretation.
 *
 * The height parameter does not currently affect driver operation.
 */
struct	lp_printer	lpconfig[] = {

/*	cols,	ht,	special_map	 	  lp# */

{	132,	66,	LPDEFAULT	} ,	/* 0 */
{	132,	66,	LPDEFAULT	} ,	/* 1 */
};

gate_t	lpgate	= 61;		/* gate for locking */
int	lpprinters = sizeof(lpconfig) / sizeof(struct lp_printer);

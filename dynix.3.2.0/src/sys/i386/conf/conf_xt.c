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

#ifndef lint
static	char	rcsid[] = "$Header: conf_xt.c 2.1 86/04/17 $";
#endif

/*
 * Configuration of tapes on the Xylogics 472
 */

/* $Log:	conf_xt.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/systm.h"

#include "../mbad/mbad.h"
#include "../mbad/xt.h"

#define	XTCTLR	2		/* maximum number of controllers */

#ifdef FULLXT
/*
 * Set "xtdensel" non-zero *only* if the attached transport
 * allows density selection.
 */

int	xtdensel	= 0;
#endif FULLXT

/*
 * per drive configuration information
 */

struct	xt_unit	xtunits[] = {
/*	 controller	drive	*/
	{ ANY,		ANY },		/* xt0: any controller, any drive */
	{ ANY,		  1 },		/* xt1: any controller, drive 1 */
	{ ANY,		  0 },		/* xt2: any controller, drive 0 */
	{ ANY,		  1 },		/* xt3: any controller, drive 1 */
};

#define	XTUNITS	(sizeof(xtunits) / sizeof(struct xt_unit))

/*
 * Static configuration information.
 *
 * DO NOT CHANGE ANYTHING BELOW THIS LINE.
 */

/*
 * per unit information
 */

int	xtmaxunit = XTUNITS;		/* number of units */
struct	buf	cxtbuf[XTUNITS];	/* headers for commands */
struct	buf	xtutab[XTUNITS];	/* heads of unit active queues */
struct	xt_softc xt_softc[XTUNITS];	/* software state */

/*
 * per controller information
 */

int	xtmaxctlr = XTCTLR;		/* number of controllers */
struct	buf	xttab[XTCTLR];		/* heads of active queues */
struct	xt_ctlr	xtctlr[XTCTLR];		/* software state */

/*
 * semaphores, locks, and gates
 */

gate_t	xtgate	= 62; 			/* lowest level lock */
lock_t	xtlock[XTCTLR];			/* lock per controller */

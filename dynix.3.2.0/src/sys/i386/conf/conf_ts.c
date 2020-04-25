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
static	char	rcsid[] = "$Header: conf_ts.c 2.3 90/03/06 $";
#endif

/*
 * cont_ts.c - SCSI tape device driver configuration file
 */

/* $Log:	conf_ts.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../sec/sec.h"			/* SCSI common data structures */
#include "../machine/ioconf.h"		/* IO Configuration Definitions */
#include "../sec/ts.h"			/* driver local structures */

/*
 * Configure the device's tuning parameters.
 *
 * The number to the far right will be the unit number portion of the
 * devices major/minor pair.
 *
 *	The bits field is or'd into scsi[1] of the command block to
 *	determine whether the immediate bit is set or not
 *	(adsi requires this set).
 */
struct ts_bconf tsbconf[] = {		/*
buf_sz,	cflags,			bits */
{64,	TSC_AUTORET,		1}, /*0*/
{64,	TSC_AUTORET,		1}, /*1*/
{64,	TSC_AUTORET,		1}, /*2*/
{64,	TSC_AUTORET,		1}, /*3*/
};

gate_t	tsgate = 57;		/* gate for this device driver */
int 	tsspintime = 2000000;

/*
 * DON'T CHANGE ANY THING BELOW THIS LINE OR ALL BETS ARE OFF!
 */
int	tsmaxndevs = sizeof(tsbconf)/sizeof(struct ts_bconf);
int	tssensebuf_sz = 32 * sizeof(char);

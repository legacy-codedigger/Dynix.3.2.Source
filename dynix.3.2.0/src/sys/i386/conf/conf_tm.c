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
static	char	rcsid[] = "$Header: conf_tm.c 1.4 90/11/13 $";
#endif

/*
 * conf_tm.c 
 * 	SSM/SCSI tape device driver configuration file.
 */

/* $Log:	conf_tm.c,v $
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
#include "../h/scsi.h"
#include "../ssm/ssm_scsi.h"		/* SCSI common data structures */
#include "../ssm/ioconf.h"		/* IO Configuration Definitions */
#include "../ssm/tm.h"			/* driver local structures */

/*
 * Configure the device's tuning parameters.
 *
 * The number to the far right will be the unit 
 * number portion of the devices major/minor pair.
 *
 * The bits field is or'd into scsi[1] of the command 
 * block to determine whether the immediate bit is set 
 * or not (adsi requires this set).
 */
struct tm_bconf tm_bconf[] = {
	/* buf_sz,	cflags,		 bits */
	{ 64, TMC_AUTORET, 1 }, 	/*0*/
	{ 64, TMC_AUTORET, 1 }, 	/*1*/
	{ 64, TMC_AUTORET, 1 }, 	/*2*/
	{ 64, TMC_AUTORET, 1 }, 	/*3*/
};

int tm_max_ndevs = sizeof(tm_bconf) / sizeof(struct tm_bconf);

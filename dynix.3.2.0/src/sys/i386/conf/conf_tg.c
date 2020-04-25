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
static	char	rcsid[] = "$Header: conf_tg.c 1.5 90/11/13 $";
#endif

/*
 * conf_tm.c 
 * 	SSM/SCSI tape device driver configuration file.
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
#include "../ssm/tg.h"			/* driver local structures */

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
struct tg_bconf tg_bconf[] = {
	/*vendor,product,embed, buf_sz,cflags */
	{ "HP", "88780", TGD_EMBED, 128, 0 }, 	/*0*/
	{ "HP", "88780", TGD_EMBED, 128, 0 }, 	/*0*/
	{ "HP", "88780", TGD_EMBED, 128, 0 }, 	/*0*/
	{ "HP", "88780", TGD_EMBED, 128, 0 }, 	/*0*/
};

int tg_max_ndevs = sizeof(tg_bconf) / sizeof(struct tg_bconf);

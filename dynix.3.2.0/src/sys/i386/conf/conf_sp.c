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
static	char	rcsid[] = "$Header: conf_sp.c 1.1 90/06/21 $";
#endif

/* $Log:	conf_sp.c,v $
*/

/*
 * conf_sp.c
 *	SSM line printer binary configuration file.
 */

/* $Log:	conf_sp.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/systm.h"
#include "../ssm/ioconf.h"
#include "../ssm/sp.h"	

/*
 * For details on configuration, see sp(4).  The special_map field is
 * defined to contain one of:
 *
 * SPDEFAULT: 		Align the form on device open and close and
 *			interpret newlines, backspaces, tabs and form feeds.
 * SPCAPS:		Upper case only mode - printer assumed to have a
 *			small character set.
 * SPRAW:		Pass all 8 bits to the printer without interpretation.
 *
 * The interface field is defined to contain
 * 	0		- Centronics printer interface
 *	1		- Data Products printer interface
 */
struct	sp_printer	spconfig[] = {

/*	cols,	special_map,	interface	 lp# */

{	132,	SPDEFAULT,	0	} ,	/* 0 */
};

int	spprinters = sizeof(spconfig) / sizeof(struct sp_printer);

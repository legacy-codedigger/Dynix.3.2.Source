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
static char rcsid[]= "$Header: conf_co.c 2.0 86/01/28 $";
#endif lint

/*
 * co_conf.c
 *
 * This file contains the binary configuration data for the
 * SCSI/Ether console driver
 */

/* $Log:	conf_co.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../sec/sec.h"
#include "../sec/co.h"


struct co_bin_config co_bin_config[] = {
	/*gate	*/
	{ 54,	},
	{ 54,	},
	{ 54,	},
	{ 54,	},
	{ 54,	},
	{ 54,	},
	{ 54,	},
	{ 54,	},
};

int co_bin_config_count = (sizeof co_bin_config)/(sizeof co_bin_config[0]);

#ifdef	DEBUG
int co_debug = 0;
#endif	DEBUG

int coflags = EVENP|ECHO|XTABS|CRMOD;
char cospeed = B9600;

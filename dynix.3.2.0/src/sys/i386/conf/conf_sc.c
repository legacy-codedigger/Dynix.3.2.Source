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
static char rcsid[]= "$Header: conf_sc.c 1.2 90/11/13 $";
#endif lint

/*
 * conf_sc.c
 *
 * This file contains the binary configuration data for the
 * SSM console driver
 */

/* $Log:	conf_sc.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../ssm/sc.h"

u_long scrxtime = 20;				/* Rx timeout 20 ms */ 
int scflow   = CCF_XOFF;			/* Flow control */
int scflags =  EVENP|ECHO|XTABS|CRMOD;		/* Line settings */
char scspeed = B9600;				/* Baud rate */

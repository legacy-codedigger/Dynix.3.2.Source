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
static	char	rcsid[] = "$Header: conf_tty.c 2.1 89/08/18 $";
#endif

/*
 * conf_tty.c
 * 	Line discipline switch table.
 */

/* $Log:	conf_tty.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/conf.h"

int	nodev();
int	nulldev();
int	nullioctl();

int	ttyopen(), ttylclose(), ttread(), ttwrite(), ttselect();
int	ttstart(), ttyinput();

struct	linesw linesw[] =
{
	ttyopen,	ttylclose,	ttread,		ttwrite,	/* 0 */
	nullioctl,	ttselect,	ttyinput,	ttstart,
	ttyopen,	ttylclose,	ttread,		ttwrite,	/* 1 */
	nullioctl,	ttselect,	ttyinput,	ttstart,
};

int	nldisp = sizeof (linesw) / sizeof (linesw[0]);

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
static	char	rcsid[] = "$Header: emon_globals.c 2.4 87/04/11 $";
#endif

/*
 * $Log:	emon_globals.c,v $
 */

/*** this module contains the global emon values */

#include "emon.h"

int if_debug;			/* debug flag */

int reverse;			/* reverse direction of lookit scan */
int dumpit;			/* dump w/o interacting */
int autoprint;			/* don't ask for print buffer just doit */
char savechar;			/* to save print(y) response char */

char tinput[80];
int tindex;

struct wbuf * wbuf_begin;	/* local offset into wbuf */

int wnumbufs = 20;		/* 20 is default # buffers */

int	smbprintlots = 1;	/* flag to printlots of smb stuff */

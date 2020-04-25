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
static	char	rcsid[] = "$Header: conf_st.c 2.4 87/05/08 $";
#endif

/*
 * conf_st.c
 *	Systech binary configuration file.
 */

/* $Log:	conf_st.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/tty.h"			/* Struct tty and bauds */
#include "../h/systm.h"
#include "../machine/ioconf.h"		/* IO Configuration Definitions */
#include "../mbad/mbad.h"		/* Multibus interface definitions */
#include "../mbad/st.h"			/* Driver local data structures */

/* Initial line state */
stflags = (EVENP|ODDP);
stspeed = B4800;

/*
 * Below are parameters that can be modified
 * to change driver actions.
 */
gate_t	stgate	= 61;		/* mutex gate */
int	ststopbits = STB_MSTOP1;/* number of stop bits on output */
int	stfifotimeout = 100000;	/* aprox. 1/2 sec timeout */
int	sttimerrate = STT_88;	/* rate sttimer() is called */
int	sttimerave = 30;	/* amount rate changes each tick */
int	stvdwait = 5;		/* spins in stintr() to wait for valid data */
int	stprintoverflow = 0;	/* non-zero prints message on input overflow */
/*
 * Block input low water / high water table.
 * SLOW mode is a block input with a termination count of 1. 
 * Fast mode is a block input with a termination count of CBSIZE.
 * For baud rates less than 600, block input is never a win
 * so we set up the table so it can't happen.
 */
struct	stlh stlh[16] = {
	{  25,    50 },		/* 0 */
	{ 100, 10000 },		/* 50 */
	{ 100, 10000 },		/* 75 */
	{ 100, 10000 },		/* 110 */
	{ 100, 10000 },		/* 134 */
	{ 100, 10000 },		/* 150 */
	{ 100, 10000 },		/* 200 */
	{ 100, 10000 },		/* 300 */
	{ 100, 10000 },		/* 600 */
	{   3,     6 },		/* 1200 */
	{   4,     9 },		/* 1800 */
	{   6,    12 },		/* 2400 */
	{  12,    21 },		/* 4800 */
	{  14,    25 },		/* 9600 */
	{  18,    31 },		/* 19200 */
	{  25,    50 },		/* 38400 */
};

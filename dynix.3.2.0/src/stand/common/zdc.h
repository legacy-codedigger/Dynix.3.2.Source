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

/*
 * $Header: zdc.h 1.2 86/04/23 $
 *
 * zdc.h
 *	Stand-alone ZDC definitions and data structures.
 */

/* $Log:	zdc.h,v $
 */

/*
 * zd(unit, partition)
 *
 * unit:
 *	0x0F	drive number on controller (0-f).
 *	0x70	controller number (0-7).
 */
#define	ZDC_DRIVE(u)	((u) & 0xf)
#define	ZDC_CTRLR(u)	(((u) >> 4) & 0x7)

/*
 * Drive supported debug options
 */
#define	ZD_BSFDEBUG	0x01
#define	ZD_DUMPCBDEBUG	0x02

/*
 * zdinfo structure is used by formatter to determine
 * disk description data (thus, channel configuration) for a given
 * disk drive type.
 */
struct zdinfo {
	caddr_t		zi_name;		/* Drive name - for humans */
	struct zdcdd	zi_zdcdd;		/* disk description */
};

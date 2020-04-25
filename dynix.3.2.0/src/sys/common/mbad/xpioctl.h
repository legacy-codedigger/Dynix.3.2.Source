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
 * $Header: xpioctl.h 2.0 86/01/28 $
 *
 * Xylogics 450 multibus SMD disk controller: definitions for ioctl
 */

/* $Log:	xpioctl.h,v $
 */

/*
 * ioctl commands
 */

struct	xp_ioctl {
	short	x_cyl;			/* desired cylinder */
	short	x_head;			/* desired head */
	short	x_sector;		/* desired sector */
};

#define	XPIOCZAPHEADER	_IOW(p, 1, struct xp_ioctl)	/* mark sector bad */

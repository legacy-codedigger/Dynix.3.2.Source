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
 * $Header: reboot.h 2.1 87/07/22 $
 */

/* $Log:	reboot.h,v $
 */

/*
 * Arguments to reboot system call.
 */
#define	RB_AUTOBOOT	0	/* flags for system auto-booting itself */

#define	RB_ASKNAME	0x01	/* ask for file name to reboot from */
#define	RB_SINGLE	0x02	/* reboot to single user only */
#define	RB_NOSYNC	0x04	/* dont sync before reboot */
#define	RB_HALT		0x08	/* don't reboot, just halt */
#define	RB_INITNAME	0x10	/* name given for /etc/init */

#define RB_NO_CTRL	0x20	/* for FIRMWARE, don't start controller */
#define RB_NO_INIT	0x40	/* for FIRMWARE, don't init system */
#define RB_AUXBOOT	0x80	/* Boot auxiliary boot name */
#define RB_DUMP		RB_AUXBOOT
#define RB_CONFIG	0x100	/* for FIRMWARE, only build cfg table */
#define RB_NO_CACHE	0x200	/* boot with cache turned off */

#define	RB_PANIC	0	/* reboot due to panic */
#define	RB_BOOT		1	/* reboot due to boot() */

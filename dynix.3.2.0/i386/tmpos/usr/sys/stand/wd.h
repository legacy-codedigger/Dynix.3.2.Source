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

/* $Header: wd.h 1.2 90/09/13 $ */

/*
 * Useful macros and driver structures used
 * to operate the SSM SCSI standalone disk device.
 */

/* $Log:	wd.h,v $
 */


#define WD_ADDRALIGN 	8	/* align I/O to 8-byte boundaries */

#define WD_LUN_SHIFT	0x05		/* fill lun into scsi_cb.sh.cb_scmd */
#define WD_MAX_XFER	0x10000		/* SCSI xfer size limit, in blocks */

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

/* $Header: ssm.h 1.1 90/07/06 $ */

/*
 * Definitions for suport routines for SSM standalone device drivers.
 */

/* $Log:	ssm.h,v $
 */

/* 
 * Definitions for alignment required
 * for efficiency and boundaries that
 * the SSM CPU cannot cross.
 */
#define	SSM_ALIGN_BASE	16		/* Align cbs to 16-byte boundaries */
#define	SSM_ALIGN_XFER	16		/* Align xfers to 16-byte boundaries */
#define	SSM_BAD_BOUND	(1 << 20)	/* CB's can't cross this boundary */

/*
 * Information structure for standalone SSM devices. 
 */
struct ssm_sinfo {
	struct scsi_cb	*si_cb;		/* I/O CB to use for this device */
	long		si_id;		/* I.D. Number returned by SSM */
	u_char		si_unit;	/* SCSI addr of device (0..63)	 */
	u_char		si_slic;	/* Slic address of the ssm board */
	long		si_version;	/* SSM F/W version from diags    */
};

/*
 * Interfaces available from ssm.c
 */
extern char *ssm_alloc();
extern ssm_get_devinfo();
extern ssm_print_sense();

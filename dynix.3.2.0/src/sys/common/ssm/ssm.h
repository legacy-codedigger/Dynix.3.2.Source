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

/* $Header: ssm.h 1.3 90/08/09 $ */

/*
 * ssm.h
 *      Systems Services Module (SSM) definitions
 */

/* $Log:	ssm.h,v $
 */

/*
 * SSM_vec is located in ssm.c.  It is a bit vector containing a
 * bit for each SSM for which configuration is attempted, i.e.,
 * bit 0 corresponds to ssm0.  The bit is set that SSM is alive
 * and clear otherwise.  The following macro tests the bit
 * setting for the specified SSM.
 */
#define SSM_EXISTS(index)	(SSM_vec & (1 << (index)))

/*
 * Memory allignment definitions for the SSM.
 * Data structures transferred by the SSM CPU 
 * cannot cross a megabyte boundary and should
 * be 16 byte aligned.  DMA transfers are most
 * efficient when located upon 16 byte boundaries.
 */
#define	SSM_ALIGN_BASE	16		/* Align host/SSM shared data 
					 * structures to 16-byte boundary */
#define	SSM_ALIGN_XFER	16		/* Align xfers to 16-byte boundary */
#define	SSM_BAD_BOUND	(1 << 20)	/* Xfer can't cross meg. boundary */

#ifdef KERNEL
/* 
 * Global definitions associated with the SSM.
 */
extern	struct	ssm_conf ssm_conf[];	/* Descriptions set up by config(1).
					   Output by config in the build 
					   directory file ioconf.c */
extern struct ssm_desc *SSM_desc;	/* Array of SSM descriptors. */
extern struct ssm_desc *SSM_cons;	/* SSM descriptor of console board. */
#ifndef i386
extern	gate_t	ssmgate;		/* Defined in conf_ssm.c */
#endif i386

/* Interfaces defined in ssm.c */
extern int conf_ssm(), probe_ssm_devices();
extern int ssm_map();

#endif KERNEL

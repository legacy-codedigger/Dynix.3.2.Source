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
 * $Header: sm.h 2.0 86/01/28 $
 *
 * sm.h
 *	SCSI driver definitions.
 */

/* $Log:	sm.h,v $
 */

/*
 * Information structure - one per channel.
 */
struct sm_info{
	struct sec_dev *sm_desc;	/* config structure */
	struct sm_stats {
		int	sm_stats_cmd;		/* number of commands */
		int	sm_stats_ioctls;	/* number of ioctls */
		int	sm_stats_xfers;		/* number of transfers */
	} sm_stats;
};

/*
 * struct sm 1 per system
 */
struct sm {
	int sm_stat;			/* status from read and write coms */
	int sm_busy;			/* a flag to make sure we have a req */
	struct sm_info *sm_info[MAXNUMSEC];	/* pointers to sm_info structures */
	struct sec_dev_prog sm_dp;		/* device program */
	sema_t sm_sema;			/* lockout semaphore */
	sema_t sm_done;			/* io completion semaphore */
	struct sm_info *sm_cons;	/* Pointer to console sm_info struct */
	u_char sm_buf[512];		/* data buffer */
};

/*
 * Probe Responses and misc
 */
#define	SM_FOUND	1		/* a good and valid device */
#define SM_TRUE		1
#define SM_FALSE	0

/*
 * SCSI driver macros
 */
#define SM_UNIT(dev)	sm->sm_info[dev]->sm_desc->sd_sec_idx
#define SM_CONS		0x40		/* is console bit */

/*
 * SCSI Device Commands
 */
#define	SMC_WRITE		0x2A
#define	SMC_READ		0x28

/*
 * SCSI Board Address limits
 */
#define STARTSMMEM		0x8000
#define ENDSMMEM		0xffff

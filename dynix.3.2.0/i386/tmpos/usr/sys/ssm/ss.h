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
 * $Header: ss.h 1.1 90/06/21 $
 *
 * ss.h
 *	SSM internal device driver definitions.
 */

/* $Log:	ss.h,v $
 */

/*
 * Structure in which device statistics are kept.
 */
struct sm_stats {
	int	ss_stats_cmd;		/* number of commands */
	int	ss_stats_ioctls;	/* number of ioctls */
	int	ss_stats_xfers;		/* number of transfers */
};

/*
 * Information structure - one per ss-device.
 */
struct ss_info{
	struct  ssm_dev *ss_desc;	/* Config structure */
	struct sm_stats ss_stats;	/* Device statistics */
};

/*
 * Per system structure for SSMs.
 */
#define SSBUFSZ		1024		/* Size of I/O data buffer */
struct ss {
	struct ss_info *ss_info[MAXNUMSSM];/* pointers to ss_info structures */
	sema_t ss_sync;			/* lockout semaphore */
	struct ss_info *ss_cons;	/* Pointer to console sm_info struct */
	u_char ss_buf[SSBUFSZ];		/* data buffer */
};

/* 
 * The following structure must be consistent with 
 * the struct sec_mem in sec.h.  Its defined here
 * to avoid including SCED definitions.
 */
#define sec_mem		ssmlog
struct ssmlog {
	char *ml_buffer;		/* Address of log buffer */
	char *ml_nextchar;		/* Next free char in buffer */
	short ml_size;			/* Buffer size */
	short ml_nchars;		/* Valid chars in buffer */
}; 

/*
 * SSM driver macros
 */
#define SS_UNIT(dev)	SS->ss_info[dev]->ss_desc->sdv_ssm_idx

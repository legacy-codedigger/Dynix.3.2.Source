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

/* $Header: sec_ctl.h 2.0 86/01/28 $
 *
 * This file contains the definitions of the common structures
 * used by SCSI/Ether drivers.
 */

/* $Log:	sec_ctl.h,v $
 */

/*
 * struct sed_dc - structure to maintain the queue's with the macros
 * below.
 */
struct seddc {
	/*
	 * device program management.
	 */
	struct	sec_cib *dc_cib;		/* cib ptr */
	struct	sec_progq *dc_diq;		/* diq ptr */
	struct	sec_progq *dc_doq;		/* doq ptr */
	int	dc_dsz;			/* doq size */
	int	dc_dp;			/* number of device programs */
	int	dc_dfree;
	struct	sec_dev_prog *dc_devp;	/* head of the device program ring */
	struct	sec_dev_prog *dc_sense;	/* aux device program for sense info */
	/*
	 * iat management.
	 */
	struct	sec_iat *dc_iat;	/* Start of iats */
	struct	sec_iat	*dc_istart;	/* current useable iat ring position */
	int	dc_isz;			/* iat ring size */
	int	dc_ihead;		/* iat head */
	int	dc_itail;		/* iat tail */
	int	dc_ifree;		/* current ring allocation */
};

/*
 * One of these exists per active SCSI/Ether Controller.
 * Filled in by conf_scsi().
 */
struct sec_desc {
	u_int			sec_diag_flags;		/* flags from powerup */
	char			sec_target_no;		/* host adapter target # */
	u_char			sec_slicaddr;		/* slic address of SEC */
	u_char			sec_ether_addr[6];	/* ether address */
	u_char			sec_is_cons;		/* is this the console? */
	struct sec_powerup	*sec_powerup;		/* initial powerup queues */
	u_short			sec_version;		/* firmware version */
};

/*
 * sec_pup_info - provide information for the standalone routines
 */

struct sec_pup_info {
	u_char	sec_target;
	struct	sec_powerup *sec_pup_q;
	struct	sec_desc sec_pup_desc;
	u_int	sec_pup_status;
	struct	sec_dev_prog *sec_pup_dev_prog;
};


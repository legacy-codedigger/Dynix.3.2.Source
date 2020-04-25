/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: wd.h 1.3 91/01/14 $ */

/*
 * wd.h
 *      SCSI disk driver for SSM definitions
 */

/* $Log:	wd.h,v $
 */


/*
 * SCSI disk driver macros
 *
 * The structure of the minor number is
 * bits 0-2 are the partition table index; 3-7 is the index into
 * the binary configuration table.
 */
#define	WD_PRTCHR(dev)	('a' + VPART((dev)))
#define WD_NO_UNIT	-1		/* statistics unit num if not valid */
#define WD_LEVEL_SHIFT	0x01		/* divide by #cb's to get info index */
#define WD_LEVEL_CB	0x01		/* cb index */
#define WD_RSENSE(cb)	((struct scrsense *) (cb)->sh.cb_sense)

#define WD_PRINT_CB	1

/*
 * Locking levels
 */
#define	WDSPL		SPL5		/* level to lock wd_lock at */

/*
 * Bit patterns expected in results from INQUIRY command.  To identify
 * units on a target adaptor, byte 3 (scinq.sc_reserved )
 * must be one of the following:
 * 1 for CCS format,
 * 0 for adaptec drives.
 * Byte 0 must be SC_INQ_DIRECT 
 */
#define	WD_RES_CCS	0x01
#define	WD_RES_TARG	0x00

#define WD_END	900000

/*
 * Information structure for a SCSI disk on a SSM. There is one per
 * channel (LUN).
 */
typedef struct {
	struct	ssm_dev	*wd_ssm_desc;	/* dev desc for my ssm */
	struct	cb_desc *wd_cbdp;	/* description command blocks I own */
	struct 	vtoc	*wd_part;	/* VTOC read from disk */
	unsigned short	*wd_opens;	/* per-partition open count */
	unsigned int	*wd_modes;	/* per-partition open modes */
	struct cmptsize	*wd_compat;	/* compatibility info from conf_wd.c */
	sema_t	wd_usrsync;		/* serialize open calls */
	u_short	wd_nopen;		/* number of opens */
	u_char	wd_vtoc_read;		/* has the vtoc been read yet? */
	int	wd_iovcount;		/* number of iovects per cb */
	int	wd_flags;		/* local state flags */
	u_char	wd_devno;		/* logical SCSI_DEVNO(targ, lun) */
	int	wd_inuse;		/* number of cbs I think are in use */
	daddr_t	wd_size;		/* usable disk space */
	int	wd_retrys;		/* number of times to retry xfer */
	int	wd_statunit;		/* statistics unit number */
	struct 	timeval wd_starttime;	/* start time of busy drive */
	struct	dk *wd_dkstats;		/* statistics */
	struct	buf wd_bufh;		/* header for sorted queue */
	struct  buf wd_rbufh;           /* raw buffer for ioctls */
	lock_t 	wd_lock;		/* mutex access to this structure */
} wd_info;

/*
 * Defines for info struct state flags
 */
#define WD_IS_ALIVE	0x01		/* board passed probing */
#define WD_BAD		0x02		/* drive has died */
#define WD_REZERO	0x04		/* rezeroing drive */
#define WD_SENSE	0x08		/* getting request sense info */
#define WD_EXCLUSIVE	0x10		/* device accessed exclusively*/
#define WD_FORMATTED	0x20		/* device has just been formated */
#define WD_ALLBUSY	0x40		/* Whole-disk partition has */
				 	/* a writer open on it */

/*
 * Configuration structures.
 */
struct wd_bconf {
	struct cmptsize  *bc_part;      /* partition table pointer */
	int	bc_iovects;		/* number of iat's per cb */
	long	bc_blks_per_sec;	/* transfer rate in blocks per second */
};

#define	WD_DIAG_CYLS	0x02		/* Number of cylinders reserved */

/*
 * SCSI disk sizes
 */
#define WD_MAX_XFER		256	/* SCSI xfer size limit, in blocks */

/*
 * Length of data returned from SCSI commands
 */
#define WD_TEST	0
#define WD_READC	8
#define WD_REQSEN	13
#define WD_MODE	20
#define WDMAXDATASZ	20

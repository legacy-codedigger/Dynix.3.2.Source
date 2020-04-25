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

/* $Header: tm.h 1.3 91/03/22 $
 *
 * tm.h
 *	SSM/Streamer tape driver structure definitions.
 */

/* $Log:	tm.h,v $
 */

/*
 * A structure used for doing the buffered 
 * read and write requests.  There will be
 * one CB corresponding to each of these.
 */
struct tm_iobuf {
	char	ci_err_flag;		/* Indicates an error with the I/O */
	short	ci_nblks;		/* # blocks currently in ci_buffer */
	u_short	ci_nmaps;		/* # maps entries in ci_maps */
	u_long	*ci_maps;		/* Address of Indirect address table */
	caddr_t	ci_buffer;		/* Address of I/O buffer */
	int	ci_curbyte;		/* Current position in ci_buffer */
	char 	*ci_msg;		/* Request Sense error message */
	int	ci_key;			/* Request sense key for reporting */
	u_int	ci_slen;		/* Valid request sense data length */
	long	ci_resid;		/* Request sense residual data length */
};

/*
 * Describe a group of sequential CBs associated 
 * with a device activity.  Consists of an index 
 * to the first CB in the group from the array of 
 * CBs associated with the device, and a semaphore, 
 * whose count indicates the number of CBs in the 
 * group when non-negative or number of processes 
 * waiting for a CB otherwise (negate count).  
 * Indicies wrap around to form a queue.
 */
struct	tm_queue {
	sema_t	sq_sync;		/* Counter and waiting list */ 
	int	sq_head;		/* Index of first CB */
	int	sq_count;		/* count of CBs on queue */
};

/*
 * Information structure definition.
 * One of these per device.
 */
struct	tm_info {
	u_char	tm_devno;		/* Scsi device number of this drive */
	u_char	tm_rwbits;		/* Asynchronous SCSI modifier bits */
	u_char	tm_cflags;		/* Configuration flags for driver */
	u_char	tm_fflags;		/* State flags for driver */
	int 	tm_bufsize;		/* I/O buffer size in bytes */
	char	tm_curmode;		/* Current mode of operation */
	char	tm_openf;		/* State of driver availability */
	spl_t	tm_spl;			/* Saved priority level */
	struct	ssm_dev	  *tm_dev;	/* Device descriptor from auto-conf */
	struct	ssm_desc  *tm_ssm;	/* Associated SSM adapter descriptor */
	u_long	tm_iocount;		/* Bytes read/writmen for statistics */
	struct tm_iobuf  tm_iobuf[NCBPERSCSI]; /* An I/O buffer per CB */
	struct scsi_cb 	  *tm_cbs;	/* Address of NCBPERSCSI CBs for this
					 * device - one per tm_iobuf element */
	short  tm_part_write;		/* Index into tm_iobuf of part filled
					 * buffer for output data (-1:none) */
	sema_t tm_usrsync;		/* Sync. system call entries */
	struct tm_queue tm_free;	/* CBs available for use */
	struct tm_queue tm_active;	/* CBs awaiting SCSI cmd termination */
	struct tm_queue tm_dav;		/* CBs with read data available */
	struct tm_queue tm_gen_cmd;	/* Terminated CBs other than for
					 * SCSI READ/WRITE commands */
	lock_t	tm_lock;		/* Lock access to info structure */
	u_char	tm_type;		/* Type of tape drive */
	daddr_t	tm_blkno;		/* record number on tape */
	daddr_t tm_fileno;		/* file number on tape */
	int	tm_resid;		/* last residule written or read */
};

/*
 * Values taken by ip->tm_curmode.
 */
#define	TMM_READ	0		/* Device may read, but not write */
#define	TMM_WRITE	1		/* Device may write, but not read */
#define	TMM_GENERAL	2		/* Device may read or write */

/*
 * Values taken by ip->tm_openf.
 */
#define	TMO_CLOSED	0		/* Device is not open */
#define	TMO_OPEN	1		/* Device is open to a process */
#define	TMO_RWNDCLS	2		/* Device is rewinding, but closed */
#define	TMO_ERR		3		/* Device is open, but had an error */
#define	TMO_FLUSH	4		/* Device is open, but terminating
					   and flushing it I/O activity */

/*
 * Values taken by ip->tm_type.
 */
#define	TMT_EMULEX	0
#define	TMT_TANDBERG	1

/*
 * State flags for ip->tm_fflags.
 */
#define	TMF_EOM		0x1		/* End of media encountered */
#define	TMF_FAIL	0x2		/* Operation failed */
#define	TMF_LASTIOR	0x4 		/* Last I/O was a READ.  Skip
					   to EOF upon closing. */
#define	TMF_LASTIOW	0x8		/* Last I/O was a WRITE. Write
					   a filemark upon closing. */
#define	TMF_ATTEN	0x10		/* Unit needs attention */
#define	TMF_FIRSTOPEN	0x20		/* First OPEN of device */
#define	TMF_EOF		0x40		/* End of file encountered */

/*
 * Configurable behavior flags for ip->tm_cflags.
 */
#define	TMC_PRSENSE	0x01		/* Print all sense info */
#define	TMC_OPENFAIL	0x02		/* Fail opens if no memory */
#define	TMC_AUTORET	0x04		/* Auto retension */
#define	TMC_RSENSE	0x08		/* Print sense info associated 
					 * with read commands */
#define	TMC_WSENSE	0x10		/* Print sense info associated 
					 * with write commands */
#define	TMC_SSENSE	0x20		/* Print sense info associated 
					 * with space commands */

/*
 * Each system call entry point (OPEN, CLOSE, etc.)
 * passes along a dev_t structure describing the
 * device.  From it the minor device number can
 * be extracted, which consists of a 3-bit device
 * unit number and a 1-bit flag which indicates
 * no-rewind-on-close when set,  rewind-on-close
 * otherwise.  The following macros describe and
 * manipulate the minor device number.
 */
#define TM_UNITMAX	0x07		/* Maximum device unit number */
#define TM_NOREWFLAG	0x08		/* Flag for rewind-on-close */
#define TM_MAXMINOR	(TM_NOREWFLAG | TM_UNITMAX) /* Max minor device # */

/* 
 * Extract the device unit number from 
 * its dev_t structure. 
 */
#define TM_UNIT(dev)	(minor(dev) & 0x07)

/* 
 * Determine from the device's dev_t structure 
 * if it rewound when closed.
 */
#define TM_REWIND(dev)	(!(minor(dev) & TM_NOREWFLAG))

/*
 * Parameter block length for mode selection.
 */
#define TM_MSEL_PARMLEN		(SCSI_MODES_HLEN + SCSI_MODES_DLEN)	

/*
 * Values for the 'wait' parameter to tm_gen_scsi_cmd().
 */
#define TM_WAIT		1		/* Wait for command termination */
#define TM_NOWAIT	0		/* Don't wait for command termination */

/*
 * Values for the 'unlock' parameter to tm_pop_cb().
 */
#define TM_UNLOCK	1		/* Unlock the info structure */
#define TM_LOCKED	0		/* Don't unlock the info structure */

/*
 * Other miscellaneous definitions used by the driver.
 */
#define TM_SPL		SPL5		/* spl priority on locks and semas */
#define TM_RSENSE_LEN	0x20		/* Size of request-sense buffer */

/*
 * Configuration structures.
 */
struct tm_bconf {
	int bc_bufsz;			/* Size of buffer, in kbytes */
	u_char bc_cflags;		/* Binary configurable features */
	u_char bc_rwbits;		/* Special fusses */
};

#ifdef KERNEL

/*
 * Global references.
 */
extern struct tm_bconf tm_bconf[];	/* Binary configuration table.
					 * Declared in conf_tm.c */
extern int tm_max_ndevs;		/* Number of entries in tm_bconf.
					 * Declared in conf_tm.c */
/* Interface to tm.c */
extern tmioctl();

extern struct ssm_driver tm_driver;	/* Driver table entry from tm.c */

#ifndef i386
extern gate_t tm_gate;			/* Declared in conf_tm.c */
#endif i386

#endif KERNEL

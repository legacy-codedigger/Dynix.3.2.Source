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

/*
 * $Header: ts.h 2.6 91/04/03 $
 *
 * ts.h
 *	Streamer tape driver structure definitions.
 */

/* $Log:	ts.h,v $
 */

/*
 * Structure to combine program queue and it's size
 */
struct sec_pq {
	struct sec_progq *sq_progq;
	u_short sq_size;
};

/*
 * A structure used for doing the buffered read and write requests
 */
struct ts_iobuf {
	char	state;
	char	err_flag;
	short	nblks;
	caddr_t	buffer;
	daddr_t	start_blk;
	sema_t	io_wait;
	struct	sec_dev_prog	io_req;
	struct	sec_iat	*ts_iats;
};

struct	ts_info {
	int			ts_unit;
	u_char			ts_lun;
	u_char			ts_rwbits;
	u_char			ts_cflags;
	u_char			ts_fflags;
	char			ts_cur_mode;
	char			ts_openf;
	int			ts_bufsz;
	spl_t			ts_spl;
	int			ts_status;
	int			ts_nspace;
	int			ts_sptype;
	int			ts_spvalue;
	int			ts_curbyte;
	struct 	ts_iobuf       *ts_lobuf;
	struct	ts_iobuf       *ts_hibuf;
	struct	sec_dev_prog   *ts_saved_devp;
	struct	sec_dev	       *ts_desc;
	struct	sec_pq	  	ts_reqq;
	struct	sec_pq	  	ts_doneq;
	struct	buf	  	ts_rbufh;
	sema_t			ts_usrsync;
	sema_t			ts_iosync;
	lock_t			ts_lock;
	struct	sec_req_sense	ts_sensereq;
	u_char		       *ts_sensebuf;
	struct	sec_dev_prog	ts_genio;
	daddr_t			ts_blkno;
	daddr_t 		ts_fileno;
	int			ts_resid;
};

/*
 * Values taken by cur_mode
 */
#define	READ	0
#define	WRITE	1
#define	GENERAL	2

/*
 * Values taken by sc_openf
 */
#define	CLOSED	0
#define	OPEN	1
#define	CLOSEDREW	2	/* Close did not wait for rewind, next open 
				 * must do so. */
#define	ERR	-1

/*
 * Values taken by state
 */
#define	FREE	0
#define	VALID	1
#define	RIP	2
#define	WIP	3
#define	IODONE	4

/*
 * bits in sense info byte 0
 */
#define	SENSE_VALID	0x80
#define SENSE_ECLASS	0x70
#define SENSE_ECLASS7	0x70

/*
 * bits in sense info byte 2
 */
#define SENSE_KEY	0x0f
#define	SENSE_EOM	0x40
#define	SENSE_FM	0x80

/*
 * bits in program status for SCSI device programs
 */
#define	SEC_CHECK	0x2
#define	SEC_BUSY	0x8
#define	SEC_FW_ERR	0x40


/*
 * Macro for manipulating io_wait semaphore, for reads and writes
 */
#define	TIODONE(sp)	sema_count((sp))


/*
 * Configuration structures.
 */
struct ts_bconf {
	int bc_bufsz;			/* size of buffer, in kbytes */
	u_char bc_cflags;		/* binary configurable features */
	u_char bc_rwbits;		/* Special fusses */
};

/*
 * misc
 */
#define TS_ANYBIN	4		/* Only used during probe */
#define TSSPL		SPL5		/* spl priority on locks and semas */

/*
 * SCSI driver macros
 */
#define TS_UNITMAX	0x07		/* maximum possible unit number */
#define	TS_UNITMASK	0x07		/* mask to get unit number */
#define TS_REWMASK	0x08		/* tape size mask */
#define TS_UNIT(dev)	(minor((dev)) & TS_UNITMASK)
#define TS_REWIND(dev)	((minor((dev)) & TS_REWMASK) == 0)	
					/* Do a rewind on this one */

#define TS_RWSIZE	6		/* scsi command size for w/r's */
#define TS_SHIFT 	9 		/* 1024 */
#define TS_TEST		TS_RWSIZE	/* size of test unit ready command */

#define TSDUMP(info, dc) printf("info=%x, dc=%x\n", (info), (dc))

/*
 * Ioctl commands.
 */
#define	TSI_STATS		1
#define	TSI_CMD			2

/*
 * defines for various types of tape space functions.
 */
#define	TS_SPBLK		0x00
#define	TS_SPFM			0x01
#define	TS_SPEOD		0x03

/*
 * Flags for keeping state information around
 */
#define	TSF_EOM		0x1
#define	TSF_FAIL	0x2
#define	TSF_LSTIOR	0x4 
#define	TSF_LSTIOW	0x8
#define	TSF_ATTEN	0x10
#define	TSF_FSTOPEN	0x20
#define	TSF_EOF		0x40

/*
 * flags for configurable behavior
 */
#define	TSC_PRSENSE	0x1			/* print all sense info */
#define	TSC_OPENFAIL	0x2			/* fail opens if no memory */
#define	TSC_AUTORET	0x4			/* auto retension */
#define	TSC_RWS_SENSE	0x8			/* print sense info for
							read/write/space only */

/*
 * format of data returned by INQUIRY command
 */

struct tsinq {
	u_char tsq_devtype;	/* 0x01 for sequential access devices */
	u_char tsq_rmv;		/* 0x80 for removable media */
	u_char tsq_version;	/* 1 for ANSI SCSI compliance */
	u_char tsq_format;	/* always 0 for adaptec, and MT02 */
};

/*
 * bit patterns in INQUIRY command return data.  These define the
 * contents of byte 0 and 1, respectively, for EMULEX MT01.
 */

#define	TS_DEVTYPE	0x01
#define	TS_RMV		0x80

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
 * $Header: sd.h 2.16 91/01/14 $
 */

/*
 * sd.h
 *	SCSI driver definitions.
 */

/* $Log:	sd.h,v $
 */

/*
 * Information structure - one per channel.
 */
struct sd_info{
	/*
	 * flags, states, etc...
	 */
	int	sd_stat;		/* low memory area for return status */
	int	sd_flags;		/* Local state flags */
	int	sd_nopen;		/* number of opens */
	int	sd_retrys;		/* Configuration - max retrys */
	struct sec_dev_prog *sd_savedp;	/* saved error'd device program ptr */
	u_char	sd_lun;			/* Correct lun for this unit */
	daddr_t	sd_size;		/* usable disk space */

	/*
	 * Partition tables (VTOC-supplied and compatibility mode.)
	 */
	struct vtoc 	*sd_part;	/* VTOC read from disk */
	unsigned short	*sd_opens;	/*  per-partition open count */
	unsigned int	*sd_modes;	/*  per-partition modes */
	struct cmptsize	*sd_compat;	/* Compatibility info from conf_sd.c */
	u_char 		sd_vtoc_read;	/* Has the vtoc been read yet? */

	/*
	 * Configuration information copied out at boot time.
	 */
	int	 sd_thresh;		/* Max to take off queue in sdstart */
	int	 sd_low;		/* When to recall start from intr */
	struct	 seddc	*sd_dc;		/* Device queues and controls */
	struct	 sec_dev *sd_desc;	/* device descriptor for ta */
	u_char	 *sd_sensebuf;		/* error sense buffer */
	u_char	 *sd_sensebufptr;	/* iat of error sense buffer */

	/*
	 * Buffers real and fiction.
	 */
	char	*sd_rawbuf;		/* set at boot time to ..[sdrawsz] */
	struct	buf sd_bufh;		/* Header for strat */
	struct	buf sd_bp;		/* Header for start */
	struct	buf sd_rbufh;		/* Header for raw */

	/*
	 * Statistics.
	 */
	struct	timeval	sd_starttime;	/* for elapse calculations */
	struct	dk	*sd_dk;		/* current stats pointer */
	int		sd_stat_unit;	/* system unit # for stat's */

	/*
	 * Local data descriptors to the hardware.
	 * Allocated and initialized at boot time.
	 */
	lock_t	sd_lock;
	sema_t	sd_sema;
	sema_t	sd_usrsync;
	struct	sec_req_sense sd_statb;
};

/*
 * Configuration structures.
 */
struct sd_bconf {
	int bc_rawbuf_sz;		/* internal raw ioctl buf size */
	struct cmptsize  *bc_part;	/* partition table pointer */
	int bc_num_iat;			/* number of iat's per channel */
	int bc_low;			/* Max number before start slows */
	int bc_thresh;			/* Maximum to get off queue each start*/
	long bc_blks_per_sec;		/* transfer rate in blocks per second */
	int bc_res1;			/* reserved */
};

/*
 * misc
 */
#define SD_ANYBIN	4		/* Only used during probe */
#define SD_BASE		32		/* Start of units for SCSI */
#define SDSPL		SPL5		/* spl priority on locks and semas */
#define SD_ADDRALIGN	16		/* RAWIO must start on 16 byte bound */

/*
 * SCSI driver macros
 */
#define SD_IOCTL(bp)	(6)		/* stubbed see below */
#define SD_NO_UNIT	-1		/* No unit number allocated for stats */
#define SD_END		900000

/*
 * The following macros are only for readability and are only
 * used in isolated cases. Driver changes that bring up new problems
 * should suspect these after verifing code correctness.
 */
#define SD_CHANNEL(sd)	((sd)->sd_chan) /* disk  # */

/*
 *#define SD_IOCTL(bp)	(bp->b_resid)->rawcmd.cmdsize)
 * this macro is stubbed but it should really set the command size of
 * the requested SCSI io command.
 */

/*
 * Scsi Device States and flags
 */
#define SDS_IDLE		0		/* bufh == idle */
#define SDS_PROBE		1		/* device is probing */
#define SDS_BUSY		2		/* queue on bufh */

#define SDF_SENSE		1		/* obtaining sense info */
#define SDF_NOSTART		2		/* slow device stuffing a bit */
#define SDF_EXCLUSIVE		4		/* device accessed exclusively*/
#define SDF_FORMATTED		8		/* device has just been formated */
#define SDF_ALLBUSY		16		/* Whole-disk partition has */
						/* a writer open on it */

/*
 * Additional masks
 */
#define SDLUNMSK		0xe0		/* LUN data in SCSI cmd */
#define SDKEY			0x70		/* error class mask */

#define SD_DONE			0		/* stop */
#define SD_NOTDONE		1		/* continue */

/*
 * Ioctl commands.
 */
#define SDI_STATS		1
#define SDI_CMD			2

/*
 * Scsi Device Commands
 */
#define SDC_TEST		0x00
#define SDC_REQUEST_SENSE	0x03
#define SDC_FORMAT		0x04
#define SDC_READ		0x08
#define SDC_WRITE		0x0A
#define SDC_INQUIRY		0x12
#define SDC_MODE_SELECT		0x15
#define SDC_MODE_SENSE		0x1A
#define SDC_READ_CAPACITY	0x25
#define SDC_SPECIAL		0xFF	/* user filled out cmd struct */

/*
 * Sizes of SCSI commands
 */
#define SD_CMD6SZ	6
#define SD_CMD10SZ	10

/*
 * Length of data returned from SCSI commands
 */
#define SDD_TEST	0
#define SDD_INQ		5
#define SDD_READC	8
#define SDD_REQSEN	13
#define SDD_MODE	20
#define SDMAXDATASZ	20

/*
 * Read Capacity command returned data
 */
struct sdcap {
				/* highest addressable block on disk: */
	u_char sdc_nblocks0;	/* MSB */
	u_char sdc_nblocks1;
	u_char sdc_nblocks2;
	u_char sdc_nblocks3;	/* LSB */
				/* formatted size of disk blocks: */
	u_char sdc_bsize0;	/* MSB */
	u_char sdc_bsize1;
	u_char sdc_bsize2;
	u_char sdc_bsize3;	/* LSB */
};

/*
 * Request Sense returned data - supply support for both extended sense and
 * non-extended.
 */

struct sdreqsense {
	u_char sdr_class;	/* valid bit, error class - 0xf0 in CCS disks */
	u_char sdr_segnum;	/* segment number */
	u_char sdr_sensekey;	/* which error group */
	u_char sdr_info[4];	/* information - sometimes block # of error */
	u_char sdr_other[5];
	u_char sdr_errorcode;	/* specific error */
};

/*
 * defines which deal with request sense data
 */

#define SD_SENSEKEYMASK		0x0f		/* valid part of sdr_sensekey */
#define SD_RECOVERED		0x01		/* drive recovered error */
#define SD_UNIT_ATTN		0x06		/* Unit Attention sense key */

/*
 * Inquiry command returned data
 */

struct sdinq {
	u_char sdq_devtype;
	u_char sdq_rmb;
	u_char sdq_pad;
	u_char sdq_format;	/* always 0 for adaptec, always 1 for CCS */
};

/*
 * Mode Sense - Mode Select data
 */

struct sd_modes {
	u_char sdm_sdlength;	/* sense data length */
	u_char sdm_type;	/* medium type */
	u_char sdm_pad0;
	u_char sdm_bdlength;	/* block descriptor length */
	u_char sdm_density;	/* density code */
	u_char sdm_nblks[3];	/* number of blocks */
	u_char sdm_pad1;
	u_char sdm_blength[3];	/* block length */
				/* error recovery page - page code 1 */
	u_char sdm_pgcode;	/* page code */
	u_char sdm_pglength;	/* page length */
	u_char sdm_bits;	/* various error-recovery bits */
	u_char sdm_retry;	/* retry count */
	u_char sdm_corr;	/* correction span */
	u_char sdm_headoff;	/* head offset count */
	u_char sdm_dsoff;	/* data strobe offset count */
	u_char sdm_recov;	/* recovery time limit */
};

/*
 * mode sense/select page codes
 */

#define SDM_MODES	0x0	/* just return block descriptor */
#define SDM_ERROR	0x1	/* error recovery page */
#define SDM_CONN	0x2	/* disconnect/reconnect page */
#define SDM_FORMAT	0x3	/* format parameter page */
#define SDM_GEOM	0x4	/* rigid disk drive geometry page */
#define SDM_ALL		0x3f	/* return all of the above pages */

/*
 * sdm_bits defines
 */

#define SDE_DCR		0x1	/* Disable Correction */
#define SDE_DTE		0x2	/* Disable transfer on error */
#define SDE_PER		0x4	/* Post error */
#define SDE_EEC		0x8	/* Enable early correction */
#define SDE_RC		0x10	/* Read continuous */
#define SDE_TB		0x20	/* Transfer block */
#define SDE_ARRE	0x40	/* Automatic read reallocation enabled */
#define SDE_AWRE	0x80	/* Automatic write reallocation enabled */

/*
 * misc defines for mode sense/select
 */

#define SDM_PF		0x10	/* Page Format bit for mode select */

/*
 * macros for extracting the LBA from extended and non-extended Request
 * sense bytes.
 */

#define XgetLBA(b) \
	(u_int) (((((b[3] << 8) | b[4]) << 8) | b[5]) << 8) | b[6]

#define getLBA(b) \
	(u_int) (((b[1]<< 8) | b[2]) << 8) | b[3]

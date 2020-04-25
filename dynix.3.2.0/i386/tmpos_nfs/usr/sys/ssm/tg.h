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

#ident	"$Header: tg.h 1.5 91/03/22 $"

#ifndef	_SYS_TG_H_

/*
 * tg.h
 *	HP 88780 SCSI  tape driver structure definitions.
 */


#define	INF	(daddr_t)1000000
/*
 * Information structure definition.  One of these per 
 * tape drive.
 */

struct	tg_info {
	sema_t usrsync;			/* Sync. system call entries */
	sema_t gensync;			/* Termination sync for SCSI ops */
	int tg_bufsize;                 /* I/O buffer size in bytes */
	int avail;                      /* Number of CBs available for use */
	int next;                       /* Index of next available CB */
	u_char tg_devno;
	u_char tg_cflags;
	struct ssm_desc *ssm;
	struct scsi_cb *cbs;         /* SSM SCSI CB's for the device. */
	spl_t	spl;			/* Saved priority level */
	scac_t cmd;			/* SCSI command information */
	u_long term_id;			/* Saved magic cookie identifying
					 * this command when it terminates. */
	int status;			/* Command completion status */
	long resid;			/* Command completion residual count */
	lock_t	lock;			/* Lock access to this structure */
	char	access;			/* State of unit accessibility */
	u_char	sflags;			/* State modifier flags for driver */
	struct ssm_dev  *dev;			/* Device descriptor from auto-conf */
	u_long iocount;			/* Bytes read/written for statistics */
	struct tape_stats *stats;	/* Other statistical info */
	daddr_t blkno;			/* Current tape block number */
	daddr_t	nxrec;			/* position of end of tape, if known */
	daddr_t tg_fileno;		/* file number io tape */
};

/* Values taken by tg_info.access */
#define	TGA_CLOSED	0		/* Device is not open */
#define	TGA_OPEN	1		/* Device is open to a process */
#define	TGA_RWNDCLS	2		/* Device is rewinding, but closed */

/* State modifier flags for tg_info.sflags  */
#define	TGF_EOM		0x1		/* End of media encountered */
#define	TGF_FAIL	0x2		/* Operation failed */
#define	TGF_LASTIOW	0x4		/* Last I/O was a WRITE. Write
					 * a filemark upon closing. */
#define	TGF_LASTPOS	0x8		/* Last operation positioned the
					 * media; a subsequent close should
					 * not forward to next file. */
#define	TGF_EOF		0x10		/* End of file encountered */
#define TGF_UNLOAD	0x20		/* Tape was just unloaded */

#define TG_UNITMAX	0x07		/* Maximum device unit number */
#define TG_UNIT(x) ((x) >> 8 & 0xff)    /* Extract unit number from minor */
#define TG_FLAGS(x) ((x) & 0xf0)        /* Extract mode flags from minor */
#define TG_DENSITY(x) ((x) & 0x0f)      /* Extract mode density from minor */

/* Mode density values for 1/2 inch media software selection */

#define MTD_NONE        0x0             /* No software selection */
#define MTD_LOW         0x1             /* Lowest density */
#define MTD_MED         0x2             /* Intermediate density */
#define MTD_HIGH        0x3             /* Highest density */
#define MTD_COMPRESS    0x4             /* Highest density with Compression */

/*
 * Parameter block length for mode selection.
 */

#define TG_MSEL_PARMLEN         (SCSI_MODES_HLEN + SCSI_MODES_DLEN)

/* Mode flags */
#define TG_NOREWFLAG   0x80            /* Don't rewind on close */

#define TG_MINOR(unit,flags,density) ((unit) << 8 | (flags) | (density))
					/* Build a tape device minor number */

#define TG_MAXMINOR	TG_MINOR(TG_UNITMAX, TG_NOREWFLAG, MTD_COMPRESS)
#define TG_REWIND(dev)  (!(TG_FLAGS(minor(dev)) & TG_NOREWFLAG))
					/* Determine from the device's dev_t
					* structure if it should be rewound
					* when closed.  */
/*
 * Other miscellaneous definitions used by the driver.
 */
#define TG_SPL		SPL5		/* spl priority on locks and semas */
#define TG_VARIABLE	0		/* 'fixed' bit value for I/O commands */
#define TG_DENS_NOOP 	0x7f		/* Use front-panel density */
#define TG_RSENSE_LEN   0x28            /* Size of request-sense buffer */


/* Flag definitions for tg_bconf.bc_cflags */
#define	TGC_PRSENSE	0x01		/* Print all sense info */
#define	TGC_OPENFAIL	0x02		/* Fail opens if no memory */
#define	TGC_AUTORET	0x04		/* Auto retension */
#define	TGC_RSENSE	0x08		/* Print sense info associated 
					 * with read commands */
#define	TGC_WSENSE	0x10		/* Print sense info associated 
					 * with write commands */
#define	TGC_SSENSE	0x20		/* Print sense info associated 

/*
 * Configuration structures.
 */
struct tg_bconf {
	char *vendor;                   /* Vendor i.d. string */   
	char *product;                  /* Product i.d. string */
	u_char embedded;                /* Target adapter type */
	int bc_bufsz;                   /* Size of buffer, in kbytes */
	u_char bc_cflags;               /* Binary configurable features */
};

/* For tg_drive_info.embedded */
#define TGD_NONEMBED    0               /* Target adapter type is nonembedded */
#define TGD_EMBED       1               /* Target adapter type is embedded */

#ifdef KERNEL

/*
 * Global references.
 */
extern struct tg_bconf tg_bconf[];	/* Binary configuration table.
					 * Declared in conf_tg.c */
extern int tg_max_ndevs;		/* Number of entries in tg_bconf.
					 * Declared in conf_tg.c */
/* Interface to tg.c */
extern tgioctl();

#ifndef i386
extern gate_t tg_gate;			/* Declared in conf_tg.c */
#endif 

#endif /* INKERNEL */

#define	_SYS_TG_H_
#endif	/* _SYS_TG_H_ */

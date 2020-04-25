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

/* $Header: saio.h 2.9 90/11/08 $ */

/*
 * Header file for standalone package
 */
struct inode {
	u_short	i_flag;
	u_short	i_count;	/* reference count */
	dev_t	i_dev;		/* device where inode resides */
	u_short	i_shlockc;	/* count of shared locks on inode */
	u_short	i_exlockc;	/* count of exclusive locks on inode */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	struct	fs *i_fs;	/* file sys associated with this inode */
	struct 	icommon i_ic;	/* disk portion of inode */
};

/*
 * Io block: includes an
 * inode, cells for the use of seek, etc,
 * and a buffer.
 */
struct	iob {
	daddr_t i_debug;        /* dynamic debug flags */
	int	i_flgs;		/* see F_ below */
	struct	inode i_ino;	/* inode, if file */
	int	i_unit;		/* pseudo device unit */
	daddr_t	i_boff;		/* block offset on device */
	daddr_t	i_cyloff;	/* cylinder offset on device */
	off_t	i_offset;	/* seek offset in file */
	daddr_t	i_bn;		/* 1st block # of next read */
	char	*i_ma;		/* memory address of i/o buffer */
	int	i_cc;		/* character count of transfer */
	int	i_error;	/* error # return */
	int	i_errcnt;	/* error count for driver retries */
	int	i_errblk;	/* block # in error for error reporting */
	int	i_howto;	/* how to open */
	caddr_t	i_buf;		/* i/o buffer */
	struct	fs *i_fs;	/* file system super block info */
	char	i_fname[MAXPATHLEN];
};
#define NULL 0

#define F_READ		0x1	/* file opened for reading */
#define F_WRITE		0x2	/* file opened for writing */
#define F_ALLOC		0x4	/* buffer allocated */
#define F_FILE		0x8	/* file instead of device */
#define F_NBSF		0x10	/* no bad sector forwarding */
#define F_ECCLM		0x20	/* limit # of bits in ecc correction */
#define F_SSI		0x40	/* set skip sector inhibit */
#define F_SEVRE		0x80	/* Severe burnin (no retries, no ECC) */
/* io types */
#define	F_RDDATA	0x0100	/* read data */
#define	F_WRDATA	0x0200	/* write data */
#define F_HDR		0x0400	/* include header on next i/o */
#define F_CHECK		0x0800	/* perform check of data read/write */
#define F_HCHECK	0x1000	/* perform check of header and data */
/* special types */
#define F_PACKET	0x10000	/* packet (should be outside F_TYPEMASK) */
#define F_TAPE          0x20000 /* tape (should be outside F_TYPEMASK) */

#define	F_TYPEMASK	0xff00

/*
 * Device switch.
 */
struct devsw {
	char	*dv_name;
	int	(*dv_strategy)();
	int	(*dv_open)();
	int	(*dv_close)();
	int	(*dv_ioctl)();
	int	(*dv_lseek)();	/* SCS: for the packet drivers */
	int	dv_flags;	/* SCS: flags to indicate it is packet */
};

#define	D_PACKET	0x01	/* SCS: driver is packet driver */
#define D_DISK		0x02	/* SCS: driver is a disk driver */
#define D_TAPE		0x04	/* SCS: driver is a tape driver */

extern struct devsw devsw[];

/*
 * Drive description table.
 * Returned from SAIODEVDATA call.
 */
struct st {
	short	nsect;		/* # sectors/track */
	short	ntrak;		/* # tracks/surfaces/heads */
	short	nspc;		/* # sectors/cylinder */
	short	ncyl;		/* # cylinders */
	daddr_t	*off;		/* partition offset table */
};

/*
 * Request codes. Must be the same a F_XXX above
 */
#define	READ	1
#define	WRITE	2

#define	NFILES	2

#define RAWALIGN 16

#ifdef STANDALONE
char	*b[NIADDR];
daddr_t	blknos[NIADDR];

struct	iob iob[NFILES];
#endif

extern	int errno;	/* just like unix */

/* error codes */
#define	EBADF	1	/* bad file descriptor */
#define	EOFFSET	2	/* relative seek not supported */
#define	EDEV	3	/* improper device specification on open */
#define	ENXIO	4	/* unknown device specified */
#define	EUNIT	5	/* improper unit specification */
#define	ESRCH	6	/* directory search for file failed */
#define	EIO	7	/* generic error */
#define	ECMD	10	/* undefined driver command */
#define	EBSE	11	/* bad sector error */
#define	EWCK	12	/* write check error */
#define	EECC	13	/* uncorrectable ecc error */
#define	EHER	14	/* hard error */
#define	EHDRECC	15	/* uncorrectable header ecc error */
#define	ESO	16	/* Sector overun error - indicates header problem */

/* ioctl's -- for disks just now */
#define	SAIOHDR		(('d'<<8)|1)	/* next i/o includes header */
#define	SAIOCHECK	(('d'<<8)|2)	/* next i/o checks data */
#define	SAIOHCHECK	(('d'<<8)|3)	/* next i/o checks header & data */
#define	SAIONOBAD	(('d'<<8)|4)	/* inhibit bad sector forwarding */
#define	SAIODOBAD	(('d'<<8)|5)	/* enable bad sector forwarding */
#define	SAIOECCLIM	(('d'<<8)|6)	/* limit ecc correction to 5 bits */
#define	SAIOECCUNL	(('d'<<8)|7)	/* use standard ecc procedures */
#define	SAIODEVDATA	(('d'<<8)|8)	/* get device data */
#define	SAIOSSI		(('d'<<8)|9)	/* set skip sector inhibit */
#define	SAIONOSSI	(('d'<<8)|10)	/* inhibit skip sector handling */
#define	SAIOSSDEV	(('d'<<8)|11)	/* is device skip sector type? */
#define	SAIODEBUG	(('d'<<8)|12)	/* enable/disable debugging */
#define	SAIOSEVRE	(('d'<<8)|13)	/* severe burnin, no ECC, no retries */
#define	SAIONSEVRE	(('d'<<8)|14)	/* clear severe burnin */
/* Sequent added IOCTL calls */
#define SAIOSCSICMD	(('d'<<8)|15)	/* SCSI command */
#define	SAIOX450CMD	(('d'<<8)|16)	/* Xylogics 450 command */
#define	SAIOZDCCMD	(('d'<<8)|17)	/* ZDC command */
#define	SAIOSETBBL	(('d'<<8)|18)	/* Set new bad block list */
#define SAIOFIRSTSECT	(('d'<<8)|19)	/* Return first sector of usable space*/
#define	SAIOFORMAT	(('d'<<8)|20)	/* Format track command */
#define	SAIOZSETBASE	(('d'<<8)|21)	/* Set i_boff */

/* codes for sector header word 1 */
#define	HDR1_FMT22	0x1000	/* standard 16 bit format */
#define	HDR1_OKSCT	0xc000	/* sector ok */
#define	HDR1_SSF	0x2000	/* skip sector flag */

/* error code for talking to SLIC */
#define	SL_GOOD		(SL_PARITY|SL_EXISTS|SL_OK)
#define SL_NOT_OK	(-1)
#define	SL_BAD_INTR	(-2)
#define SL_BAD_PAR	(-3)
#define SL_BAD_DEST	(-4)
#define SL_PERROR	(-1)
#define SL_DERROR	(-2)
#define SL_NOTOK	(-3)

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
 * $Header: mtio.h 2.0 86/01/28 $
 *
 * mtio.h 
 * 	Structures and definitions for mag tape io control commands
 */

/* $Log:	mtio.h,v $
 */

/* structure for MTIOCTOP - mag tape op command */
struct	mtop	{
	short	mt_op;		/* operations defined below */
	daddr_t	mt_count;	/* how many of them */
};

/*
 * operations
 * 
 * Note: Device drivers depend on the ordering below.
 */
#define MTWEOF	0	/* write an end-of-file record */
#define MTFSF	1	/* forward space file */
#define MTBSF	2	/* backward space file */
#define MTFSR	3	/* forward space record */
#define MTBSR	4	/* backward space record */
#define MTREW	5	/* rewind */
#define MTOFFL	6	/* rewind and put the drive offline */
#define MTNOP	7	/* no operation, sets status only */
#define MTERASE	8	/* Erase from current position to eot */
#define MTRET	9	/* Retention the tape (streamers) */
#define MTSEOD	10	/* Space to end of data */
#define MTNORET	11	/* don't retention the tape this time */

/*
 * Structure for MTIOCGET - mag tape get status command
 */

struct	mtget	{
	short	mt_type;	/* type of magtape device */
	/*
	 * The following two registers are grossly device dependent
	 */
	short	mt_dsreg;	/* ``drive status'' register */
	short	mt_erreg;	/* ``error'' register */
	/*
	 * End device-dependent registers
	 */
	short	mt_resid;	/* residual count */
	/* 
	 * The following two are not yet implemented on all devices
	 */
	daddr_t	mt_fileno;	/* file number of current position */
	daddr_t	mt_blkno;	/* block number of current position */
};

/*
 * Constants for mt_type byte
 */
#define	MT_ISTS		0x01
#define	MT_ISHT		0x02
#define	MT_ISTM		0x03
#define	MT_ISMT		0x04
#define	MT_ISUT		0x05
#define	MT_ISCPC	0x06
#define	MT_ISAR		0x07
#define MT_ISXT		0x08	/* Cipher streamer */
#define MT_ISST		0x09	/* scsi streamer tape */
#define MT_ISTB		0x0a	/* scsi block tape */

/* mag tape io control commands */
#define	MTIOCTOP	_IOW(m, 1, struct mtop)		/* do a mag tape op */
#define	MTIOCGET	_IOR(m, 2, struct mtget)	/* get tape status */

#ifndef KERNEL
#define	DEFTAPE	"/dev/rmt12"	/* default tape for "mt"commands */
#endif

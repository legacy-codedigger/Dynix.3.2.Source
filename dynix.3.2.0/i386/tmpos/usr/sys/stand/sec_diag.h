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

/* $Header: sec_diag.h 2.0 86/01/28 $
 *
 * SEC DIagnostics Header file
 *
 * Defines structures for the SEC diagnostics tracks.
 * Uses structures defined in scsi.h.
 */

/*
 * Adaptec's SCSI Messages
 */
#define M_COMMAND_COMPLETE	0x00
#define M_SAVE_DATA_PTR		0x02
#define M_RESTORE_PTRS		0x03
#define M_DISCONNECT		0x04
#define M_INIT_DETECT_ERR	0x05
#define M_ABORT			0x06
#define M_MESSAGE_REJECT	0x07
#define M_NO_OPERATION		0x08
#define M_LINK_CMD_COMPL	0x0A
#define M_LINK_CMD_COMPL_FLAG	0x0B
#define M_BUS_DEVICE_RESET	0x0C
#define M_IDENTIFY_START	0x80
#define M_IDENTIFY_STOP		0xFF

/*
 * Adaptec's SCSI Status Byte Codes
 */
#define SBC_GOOD		0x00
#define SBC_CHECK		0x02
#define SBC_COND_MET		0x04
#define SBC_BUSY		0x08
#define SBC_INTERMED_STAT	0x10
#define SBC_RESERV_CONFL	0x18

/*
 * Adaptec's SCSI Error Codes
 */
#define E_NO_SENSE		0x00
#define E_NO_INDEX_SIGNAL	0x01
#define E_NO_SEEK_COMPLETE	0x02
#define E_WRITE_FAULT		0x03
#define E_DRIVE_NOT_READY	0x04
#define E_NO_TRACK_00		0x06
#define E_ID_CRC_ERR		0x10
#define E_UNCORR_DATA_ERR	0x11
#define E_NO_ID_ADDR_MARK	0x12
#define E_NO_DATA_ADDR_MARK	0x13
#define E_RECORD_NOT_FOUND	0x14
#define E_SEEK_ERR		0x15
#define E_DATA_CHK_NO_RETRY	0x18
#define E_ECC_ERR_VERIFY	0x19
#define E_INTERLEAVE_ERR	0x1A
#define E_SELF_TEST_FAILED	0x1B
#define E_DEFECTIVE_TRK		0x1E
#define E_BAD_COMMAND		0x20
#define E_BAD_BLK_ADDR		0x21
#define E_VOLUME_OVERFLOW	0x23
#define E_BAD_ARGUMENT		0x24
#define E_BAD_LOGICAL_UNIT	0x25
#define E_USAGE_CNTR_OV		0x2C
#define E_INIT_DETECTED_ERR	0x2D
#define E_SCSI_OUT_PARITY_CHK	0x2E
#define E_ADAPTER_ERR		0x2F

/*
 * Fujitsu-Specific Definitions
 */
#define NSRF	11	/* nbr surfaces/disk */
#define NTRK	754	/* nbr tracks/surface */

/*
 * Adaptec-Specific Disk Definitions
 */
#define NBTN256	32	/* nbr 256-byte blocks/track, No Interleave */
#define NBTI256	33	/* nbr 256-byte blocks/track, Interleaved */
#define NBTN512	17	/* nbr 512-byte blocks/track, No Interleave */
#define NBTI512	18	/* nbr 512-byte blocks/track, Interleaved */
#define NBTN1024 9	/* nbr 1024-byte blocks/track, No Interleave */
#define NBTI1024 9	/* nbr 1024-byte blocks/track, Interleaved */

#define BO_256	150	/* actual byte offset -  256-byte blocks */
#define BO_512	150	/* actual byte offset -  512-byte blocks */
#define BO_1024	150	/* actual byte offset - 1024-byte blocks */

#define BB_256	322	/* actual bytes/block -  256-byte blocks */
#define BB_512	578	/* actual bytes/block -  512-byte blocks */
#define BB_1024	1090	/* actual bytes/block - 1024-byte blocks */

#define MINBLK	256	/* minimum block size */
#define MAXBLK	1024	/* maximim block size */

#define UNIX_BLK_SIZE	512	/* UNIX default block size */
#define UNIX_BLK_TRK	NBTN512	/* UNIX default block size */
#define UNIX_INTERLEAVE	1	/* UNIX default interleave */

/*
 * Data that must be determined for each lun
 *	- and redetermined if the lun is reformatted
 */
struct z {
	u_short	bb_nbr;		/* nbr bad blocks in bb list */
	u_int	bbl_size;	/* nbr bytes in bbl blocks */
	u_short	blk_clust;	/* block cluster for this disk */
	u_int	blk_cyl;	/* disk's actual blocks/cylinder */
	u_int	blk_disk;	/* disk's actual blocks/disk */
	u_int	blk_size;	/* disk's actual block size */
	u_int	blk_trk;	/* disk's actual blocks/track */
	u_int	diag_start;	/* starting block of the diag tracks */
	u_int	diag_bbl;	/* starting block for bad block list */
	u_int	diag_stop;	/* last block of the diag tracks */
	u_short	interleave;	/* positive ==> known interleave factor */
	u_short	nbr_heads;	/* disk's actual nbr of heads */
	u_short	nbr_cyls;	/* disk's actual nbr of cylinders (& tracks) */
	u_short	nbr_spares;	/* positive ==> known nbr of spares/track */
	u_char	no_format;	/* TRUE ==> disk's format is blown */
	u_char	diag_hit;	/* TRUE ==> diag tracks need re-write */
	u_char	db_in;		/* TRUE ==> db is in memory ok */
};

/*
 * Diagnostic Block Structure
 */
struct db {
	u_short	db_magic;	/* magic nbr */
	u_short	db_chksum;	/* block checksum */
	u_int	db_blkno;	/* block nbr */
	union	{
		u_char db_pattern;		/* start of test pattern */
		struct {
			struct scsi_modes db_modes;	/* mode select data */
			struct scsi_fmt	  db_format;	/* format data */
			struct z          db_data;	/* misc diag data */
		} db_f;
	} db_u;
};

/*
 * The db_f structures are ordered according to their likelihood to change:
 *	scsi_modes probably won't ever change;
 *	scsi_fmt may change, if nothing more than that MAX_DEFECTS may change;
 *	z is a hodgepodge, and will surely change.
 */

/* Handy References */
#define db_pat	db_u.db_pattern
#define db_mds	db_u.db_f.db_modes
#define db_fmt	db_u.db_f.db_format
#define db_z	db_u.db_f.db_data

#define DBSIZE	(((int)sizeof(struct db) + MAXBLK - 1) & ~(MAXBLK - 1))

#define MAGIC_DIAG	0xD001	/* magic nbr for diagnostic blocks */
#define MAGIC_BBL	0xD072	/* magic nbr for bad block list blocks */
#define CHECKSUM	0xFEDC	/* target checksum */

/*
 * static bad block data
 */
struct bb {
	u_int	bb_cyl;		/* cylinder */
	u_char	bb_head;	/* head */
	u_int	bb_bytes;	/* bytes from index */
	u_int	bb_lba;		/* logical block addr */
	u_char	bb_type;	/* type of bb data */
};

/*
 * bb[].bb_type flags
 */
#define BB_TYPE_CHB	0	/* cyl/hd/byte - normal specification */
#define BB_TYPE_CH	1	/* cyl/hd - one track */
#define BB_TYPE_C	2	/* cyl - one cyl */

/*
 * SCSI-Specific Constants
 */
#define C00SIZE		6		/* class 00 commands */
#define C01SIZE		10		/* class 01 commands */
#define LUN_SHIFT	5		/* logical unit nbr shift count */
#define LUN_MASK	0xE0		/* logical unit nbr mask */
#define LBA_MASK	0x1FFFFF	/* logical block addr mask (class 00) */
#define SIZE_BUFFER	2048		/* nbr bytes in Buffer RAM */
#define SIZE_DIAG	0x104		/* nbr bytes in Diagnostic Data */
#define SIZE_INQUIRY	5		/* nbr bytes in Inquiry Response */
#define SIZE_USAGE	9		/* nbr bytes in Usage Data */
#define SIZE_MODES_1	12		/* nbr bytes without drive param list */
#define SIZE_MODES_2	22		/* nbr bytes with drive param list */
#define SIZE_SEND_DIAG	4		/* nbr bytes in Send Diagnostic data */
#define DEF_THRESH	0		/* default check counter threshold */
#define SCSI_MODES_SRATE	2	/* max m_srate */

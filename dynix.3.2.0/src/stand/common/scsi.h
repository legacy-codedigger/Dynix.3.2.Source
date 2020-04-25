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

/* $Header: scsi.h 2.7 90/11/08 $ */

/*
 * Supported SCSI commands
 */

/* class 00 commands */
#define	SCSI_TEST	0x00		/* test unit ready */
#define SCSI_REZERO	0x01		/* rezero unit */
#define SCSI_RSENSE	0x03		/* request sense */
#define SCSI_FORMAT	0x04		/* format unit */
#define SCSI_REASS	0x07		/* reassign blocks */
#define	SCSI_READ	0x08		/* read */
#define	SCSI_WRITE	0x0a		/* write */
#define	SCSI_SEEK	0x0b		/* seek */
#define SCSI_TRAN	0x0f		/* translate logical to phys */
#define SCSI_INQUIRY	0x12		/* do inquiry, note: not 4000 */
#define SCSI_WRITEB	0x13		/* write buffer */
#define SCSI_READB	0x14		/* read buffer */
#define SCSI_MODES	0x15		/* mode select */
#define SCSI_RESRV	0x16		/* reserve unit, note: not 4000 */
#define SCSI_RELSE	0x17		/* release unit, note: not 4000 */
#define SCSI_MSENSE	0x1a		/* mode sense, note: not 4000 */
#define SCSI_STARTOP	0x1b		/* start/stop unit */
#define		SCSI_START_UNIT		0x01
#define		SCSI_STOP_UNIT		0x00
#define SCSI_RDIAG	0x1c		/* receive diagnostic */
#define SCSI_SDIAG	0x1d		/* send diagnostic */
#define		SCSI_SDIAG_REINIT	0x60
#define		SCSI_SDIAG_DUMP_HW	0x61
#define		SCSI_SDIAG_DUMP_RAM	0x62
#define		SCSI_SDIAG_PATCH_HW	0x63
#define		SCSI_SDIAG_PATCH_RAM	0x64
#define		SCSI_SDIAG_SET_RD_ERR	0x65
#define			SCSI_SDIAG_RD_ERR_DEF	0x00
#define			SCSI_SDIAG_RD_ERR_NOC	0x01
#define			SCSI_SDIAG_RD_ERR_RPT	0x02

/* class 01 commands */
#define SCSI_READC	0x25		/* read capacity */
#define		SCSI_FULL_CAP		0x00
#define		SCSI_PART_CAP		0x01
#define SCSI_READ_EXTENDED	0x28
#define SCSI_WRITE_EXTENDED	0x2A

/*
 * unsupported SCSI commands
 */
#define SCSI_SET_THRESHOLD	0x10
#define SCSI_RD_USAGE_CTRS	0x11
#define SCSI_READ_EXTENDED	0x28
#define SCSI_WRITE_EXTENDED	0x2A
#define SCSI_WRITE_AND_VERIFY	0x2E
#define SCSI_VERIFY		0x2F
#define SCSI_SEARCH_DATA_EQUAL	0x31
#define SCSI_SET_LIMITS		0x33

/*
 * Tape commands
 */
#define SCSI_REWIND	0x01		/* Rewind command */
#define SCSI_RETENTION	0x02		/* Retention a tape */
#define SCSI_WFM	0x10		/* Write a file mark */
#define SCSI_SPACE	0x11		/* Space (default blocks) fwd */
#define         SCSI_SPACE_BLOCKS       0x00
#define         SCSI_SPACE_FILEMARKS    0x01
#define         SCSI_SPACE_SFILEMARKS   0x02
#define         SCSI_SPACE_ENDOFDATA    0x03

#define SCSI_FIXED_BLOCKS       0x01
#define SCSI_ERASE_LONG         0x01

#define SCSI_ERASE	0x19		/* Erase a tape */

/*
 * Sizes of data transferred for some standard commands
 */
#define SIZE_CAP	8	/* nbr bytes in Read Capacity input data */
#define SIZE_TRANS	8	/* nbr bytes in Translate input data */
#define SIZE_INQ	4	/* nbr bytes in Inquiry output data */
#define SIZE_INQ_XTND	36	/* nbr bytes in extended Inquiry data */
#define SIZE_BDESC	12	/* nbr bytes in Mode Select block descriptor */
#define SIZE_MAXDATA	36	/* nbr bytes in largest data transfer */

#define SCSI_CMD6SZ             6       /* SCSI command length */
#define SCSI_CMD10SZ            10      /* SCSI command length */
#define SCSI_CMD12SZ            12      /* SCSI command length */

/*
 * Buffer alignment for SCED dma data
 */
#define SCSI_XFER_ALIGN	8	/* align dma data on 8 byte boundaries */

/*
 * structure for SCSI mode select command  (non-CCS devices)
 */
#define SCSI_MODES_ILEN		22	/* bytes in data block */
#define SCSI_MODES_DLEN		8	/* length of extent decriptor list */

struct  scsi_modes {
			/* command list */
	u_char	m_type;		/* command type */
	u_char	m_unit;		/* upper 3 bits are unit */
	u_char	m_pad1[2];	/* reserved */
	u_char	m_ilen;		/* length of info passed */
	u_char	m_cont;		/* control byte */
			/* parameter list */
	u_char	m_pad2[3];	/* reserved */
	u_char	m_dlen;		/* length of descript list */
			/* extent descripter list */
	u_char	m_density;	/* density code */
	u_char	m_pad3[4];	/* reserved */
	u_char	m_bsize[3];	/* block size */
			/* drive parameter list */
	u_char	m_fcode;	/* format code */
	u_char	m_cyls[2];	/* cylinder count */
	u_char	m_heads;	/* data head count */
	u_char	m_rwcc[2];	/* reduced write current cylinder */
	u_char	m_wpc[2];	/* write precompensation cylinder */
	char	m_lzone;	/* landing zone position */
	u_char	m_srate;	/* step pulse output rate code */
};

/* 
 * structures for SCSI format command (non-CCS devices)
 */

struct dlist {			/* defect list entries */
	u_char	d_cyls[3];	/* cyl of defect */
	u_char	d_heads;	/* head nbr */
	u_char	d_bytes[4];	/* bytes from index */
};

#define FORMAT_BUF	1024	/* max bytes for Format Data */
#define MAX_DEFECTS	(FORMAT_BUF / sizeof(struct dlist))

struct	scsi_fmt {
			/* command list */
	u_char	f_type;		/* command type */
	u_char	f_misc;		/* 3 bit unit, data flag, complete list bits */
	u_char	f_data;		/* data pattern */
	u_char	f_ileave[2];	/* interleave */
	u_char	f_pad1;		/* reserved */
			/* defect list */
	u_char	f_full;		/* full or cyl flag: cyl not on 4000 */
	u_char	f_spares;	/* nbr spares/cyl: not on 4000 */
	u_char	f_dlen[2];	/* length of defect list blocks */
	struct dlist dlist[MAX_DEFECTS]; /* blocks of defect list */
};

/*
 * scsi_fmt.f_misc flags
 */
#define FMT_BBL_DATA	0x10		/* bbl data exists */
#define FMT_CMPLT	0x08		/* bbl data is complete */
#define FMT_USER_FMT	0x04		/* use user-supplied fmt data */
#define FMT_DATA	0x02		/* use user-supplied data pattern */
#define FMT_ALL		(FMT_BBL_DATA | FMT_CMPLT | FMT_USER_FMT | FMT_DATA)

/*
 * scsi_fmt.f_data
 */
#define FMT_PAT		0x6D		/* worst winchester data */

/*
 * scsi_fmt.f_full flags
 */
#define	FMT_FULL	0x00		/* complete drive */
#define	FMT_CYL		0x01		/* single cylinder: not on 4000 */

/*
 * scsi_fmt.f_code
 */
#define FMT_FCODE	0x01		/* must be 1 */

/*
 * Numbering for the scsi stand alone device drivers major numbers
 * and numbering macros header file.
 *
 * Assumes sec.h included prevously.
 *
 * UNIT: Unit number, must be in the range of 0-255 inclusive.
 * 	bits 0-2: unit number on the hardware (up to 8)
 *	bits 3-5: controller select (target adapter) (up to 8)
 *	bits 6-8: drive  type (up to 8), index into configuration table 
 *	bits 9-11: scsi board number
 *
 * OFFSET: Offset specification, if in the range of 0-7, indexes into
 *	partition table otherwise OFFSET is the actual block offset
 *	or in the case of tape is the file number to space to 
 *	on each open of the device.
 */

/*
 * Unit number macros
 */
#define SCSI_UNIT(x)	(((x)>>0)&7)
#define	SCSI_TARGET(x)	(((x)>>3)&7)
#define SCSI_TYPE(x)	(((x)>>6)&7)
#define SCSI_BOARD(x)	(((x)>>9)&7)
#define SCSI_DEVNO(x)    (SCSI_TARGET((x))<<3) + (SCSI_UNIT((x)))
/*
 * SCSI command termination status byte macros.
 */
#define SCSI_GOOD(status) (((status) & 0x1E) == 0)
#define SCSI_CHECK_CONDITION(status) (((status) & 0x1E) == 0x2)
#define SCSI_CONDITION_MET(status) (((status) & 0x1E) == 0x4)
#define SCSI_BUSY(status)       (((status) & 0x1E) == 0x8)
#define SCSI_INTERMEDIATE(status)       (((status) & 0x1E) == 0x10)
#define SCSI_RES_CONFLICT(status)       (((status) & 0x1E) == 0x18)

/*
 * Inquiry command returned data.
 */
struct scinq {
	u_char sc_devtype;		/* Type SCSI device */
	u_char sc_qualif;		/* Dev type qualifier */
	u_char sc_version;		/* SCSI spec version */
	u_char sc_reserved;
	u_char sc_vlength;		/* Length of vendor unique data */
};

/*
 * Defines for scinq.sc_devtype.
 */
#define	INQ_DIRECT		0x00	/* Direct-access device */
#define INQ_SEQ			0x01	/* Sequential access device */
#define INQ_PRINT		0x02	/* Printer device */
#define INQ_PROC		0x03	/* Processor device */
#define INQ_WRONCE		0x04	/* Write-once read multiple times */
#define INQ_READONLY		0x05	/* Read only medium device */
#define INQ_NOTFOUND		0x7F	/* Logical device not found */

/*
 * Defines for scinq.sc_qualif.
 */
#define	INQ_REMOVABLE		0x80	/* Has removable media */

/*
 * Minimum structure for request sense data.
 */
struct scrsense {
	u_char	rs_error;		/* Error code and valid bit */
	u_char	rs_seg;			/* Segment Number */
	u_char	rs_key;			/* Filemark,EOM,ILI, and Sense Key */
	u_char	rs_info[4];		/* Information bytes */
	u_char	rs_addlen;		/* Additional length in bytes */
};

/*
 * Defines for scrsense.rs_error.
 */
#define	RS_VALID		0x80	/* Bit indicates error code is valid */
#define	RS_ERRCODE		0x7f	/* Mask for error code */
#define	RS_CURERR		0x70	/* Current error */
#define	RS_DEFERR		0x71	/* Deferred error */
#define	RS_VENDERR		0x7f	/* Vendor unique error code */
#define RS_CLASS_EXTEND		0x70	/* Extended class of error codes */

/*
 * Defines for scrsense.rs_key.
 */
#define	RS_FILEMARK		0x80	/* Filemark has just been read */
#define	RS_EOM			0x40	/* End of media encountered */
#define	RS_ILI			0x20	/* Incorrect block length indicator */
#define	RS_RES			0x10	/* Reserved for future use */
#define	RS_KEY			0x0f	/* Mask for Sense Key codes */

/*
 * Sense Key codes for scrsense.rs_key & RS_KEY.
 */
#define	RS_NOSENSE		0x00	/* No Sense information available */
#define	RS_RECERR		0x01	/* Recovered from error */
#define	RS_NOTRDY		0x02	/* Addressed unit not accessible */
#define	RS_MEDERR		0x03	/* Error in medium encountered */
#define	RS_HRDERR		0x04	/* Target detects hardware failure */
#define	RS_ILLREQ		0x05	/* Illegal request */
#define	RS_UNITATTN		0x06	/* Media changed or target reset */
#define	RS_PROTECT		0x07	/* Media protected against operation */
#define	RS_BLANK		0x08	/* Blank check medium error */
#define	RS_VENDUNIQ		0x09	/* Vendor unique code */
#define	RS_CPABORT		0x0a	/* Copy command aborted */
#define	RS_ABORT		0x0a	/* Command aborted */
#define	RS_EQUAL		0x0c	/* Search data found equal comparison */
#define	RS_OVFLOW		0x0d	/* Volume overflow */
#define	RS_MISCMP		0x0e	/* Source and media data mis-compare */
#define	RS_RESKEY		0x0f	/* Reserved for future use */

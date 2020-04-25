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
 * $Header: scsi.h 2.3 89/08/24 $
 *
 * Supported SCSI commands
 */

/* $Log:	scsi.h,v $
 */

/* class 00 commands */
#define	SCSI_TEST	0x00		/* test unit ready */
#define SCSI_REZERO	0x01		/* rezero unit */
#define SCSI_RSENSE	0x03		/* request sense */
#define SCSI_FORMAT	0x04		/* format unit */
#define SCSI_REASS	0x07		/* Reassign blocks */
#define	SCSI_READ	0x08		/* read */
#define	SCSI_WRITE	0x0a		/* write */
#define	SCSI_SEEK	0x0b		/* seek */
#define SCSI_TRAN	0x0f		/* translate logical to phys */
#define SCSI_INQUIRY	0x12		/* do inquiry */
#define SCSI_WRITEB	0x13		/* write buffer */
#define SCSI_READB	0x14		/* read buffer */
#define SCSI_MODES	0x15		/* mode select */
#define SCSI_RESRV	0x16		/* reserve unit */
#define SCSI_RELSE	0x17		/* release unit */
#define SCSI_MSENSE	0x1a		/* mode sense */
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
#define			SCSI_SDIAG_RD_ERR_RPT	0x01
#define			SCSI_SDIAG_RD_ERR_NOC	0x02

/* class 01 commands */
#define SCSI_READC	0x25		/* read capacity */
#define		SCSI_FULL_CAP		0x00
#define		SCSI_PART_CAP		0x01
#define SCSI_READ_DEFECTS	0x37

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
#define SCSI_RETENSION	0x02		/* Retention a tape */
#define SCSI_WFM	0x10		/* Write a file mark */
#define SCSI_SPACE	0x11		/* Space (default blocks) fwd */
#define SCSI_ERASE	0x19		/* Erase a tape */

/*
 * Sizes of data transferred for some standard commands
 */
#define SIZE_CAP	8	/* nbr bytes in Read Capacity input data */
#define SIZE_TRANS	8	/* nbr bytes in Translate input data */

#define SCSI_CMD6SZ		6	/* SCSI command length */
#define SCSI_CMD10SZ		10	/* SCSI command length */
#define SCSI_CMD12SZ		12	/* SCSI command length */
#define SCSI_MAXCMDSZ   	12	/* Maximum SCSI command length */

/*
 * The following data structure collects generic 
/*
 * structure for SCSI mode select command
 */
#define SCSI_MODES_ILEN		22	/* bytes in data block */
#define SCSI_MODES_DLEN		8	/* length of extent descriptor list */

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
			/* extent descriptor list */
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
 * structures for SCSI format command
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
	u_char	f_full;		/* full or cyl flag */
	u_char	f_spares;	/* nbr spares/cyl */
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
#define	FMT_CYL		0x01		/* single cylinder */

/*
 * scsi_fmt.f_code
 */
#define FMT_FCODE	0x01		/* must be 1 */

/*
 * SCSI_UNIT Macro:
 * 	bits 0-2: unit number on the hardware (up to 8)
 *	bits 3-5: target number (up to 8)
 *	bits 6-8: drive  type (up to 8), index into configuration table 
 *	bits 9-11: SEC board number
 *
 *	On embedded SCSI, the unit number is always 0.
 */
#define SCSI_UNIT(x)	(((x)>>0)&7)
#define	SCSI_TARGET(x)	(((x)>>3)&7)
#define SCSI_TYPE(x)	(((x)>>6)&7)
#define SCSI_BOARD(x)	(((x)>>9)&7)
#define SCSI_DEVNO(x)	(SDEV_SCSISTART + (SCSI_TARGET((x))<<3) + (SCSI_UNIT((x))) )

#define SCSI_LUNSHFT	5	/* For shifting logical unit numbers
				 * to put SCSI command. */
/*
 * Online format defintions and structures.
 */

/*
 * Read Block Limits command returned data.
 */
struct screadblk {
	u_char	rb_pad1;      	/* Reserved */
	u_char	rb_max_len[3];	/* Maximum block length (MSB...LSB) */
	u_char	rb_min_len[2];	/* Minimum block length (MSB...LSB) */
};

/*
 * Read Capacity command returned data.
 */
struct screadcap {
	                     	/* Highest addressable block on disk: */
	u_char rc_nblocks0;  	/* MSB */
	u_char rc_nblocks1;
	u_char rc_nblocks2;
	u_char rc_nblocks3;  	/* LSB */
	                     	/* Formatted size of disk blocks: */
	u_char rc_bsize0;    	/* MSB */
	u_char rc_bsize1;
	u_char rc_bsize2;
	u_char rc_bsize3;    	/* LSB */
};

/*
 * Data returned from the SCSI Inquiry command.
 * Its format consists of a header followed by
 * vendor unique data, the size in bytes of which 
 * is described in the header.
 */

/* Inquiry data header */
struct scinq_hdr {
	u_char	ih_devtype;		/* Type SCSI device */
	u_char	ih_qualif;		/* Dev type qualifier */
	u_char	ih_version;		/* SCSI spec version */
	u_char	ih_reserved;		/* 0 for adaptec, 1 for CCS */
	u_char	ih_vlength;		/* Length of vendor unique data */
};

/* For scinq_hdr.ih_devtype */
#define INQ_DIRECT		0x00	/* Direct-access device */
#define INQ_SEQ 		0x01	/* Sequential access device */
#define INQ_PRINT		0x02	/* Printer device */
#define INQ_PROC		0x03	/* Processor device */
#define INQ_WRONCE		0x04	/* Write-once read multiple times */
#define INQ_READONLY		0x05	/* Read only medium device */
#define INQ_NOTFOUND		0x7F	/* Logical device not found */

/* For scinq_hdr.ih_qualif  */
#define INQ_REMOVABLE   	0x80	/* Has removable media */

/* For scinq_hdr.ih_reserved for SCSI disks */
#define INQ_RES_CCS		0x01	/* CCS */
#define INQ_RES_TARG		0x00	/* Adaptec */

/* Here is a parameter list description for a typical device */
#define INQ_VEND		8	/* Length of vendor name field */
#define INQ_PROD		16	/* Length of product i.d. field */
#define INQ_REV 		4	/* Length of revision field */

struct scinq_dev {
	struct scinq_hdr sc_hdr;	/* Inquiry data header */
	u_char	sc_pad[3];
	char	sc_vendor[INQ_VEND];	/* Name of the vendor */
	char	sc_product[INQ_PROD];  /* Product ID */
	u_char	sc_revision[INQ_REV];  /* Product fw rev level */
};

#define INQ_LEN_DEV		sizeof(struct scinq_dev)


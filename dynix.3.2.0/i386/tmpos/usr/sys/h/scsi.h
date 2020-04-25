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

/* $Header: scsi.h 1.4 90/11/08 $ */

/* $Log:	scsi.h,v $
 */

/*
 * Supported SCSI commands
 */

/* Class 00 commands */
#define	SCSI_TEST		0x00	/* Test unit ready */
#define SCSI_REZERO		0x01	/* Rezero unit */
#define SCSI_RSENSE		0x03	/* Request sense */
#define SCSI_FORMAT		0x04	/* Format unit */
#define SCSI_REASS		0x07	/* Reassign blocks */
#define	SCSI_READ		0x08	/* Read */
#define	SCSI_WRITE		0x0a	/* Write */
#define	SCSI_SEEK		0x0b	/* Seek */
#define SCSI_TRAN		0x0f	/* Translate logical to phys */
#define SCSI_INQUIRY		0x12	/* Do inquiry, note: not 4000 */
#define SCSI_WRITEB		0x13	/* Write buffer */
#define SCSI_READB		0x14	/* Read buffer */
#define SCSI_MODES		0x15	/* Mode select */
#define SCSI_RESRV		0x16	/* Reserve unit, note: not 4000 */
#define SCSI_RELSE		0x17	/* Release unit, note: not 4000 */
#define SCSI_MSENSE		0x1a	/* Mode sense, note: not 4000 */
#define SCSI_STARTOP		0x1b	/* Start/stop unit */
#define		SCSI_START_UNIT		0x01
#define		SCSI_STOP_UNIT		0x00
#define SCSI_RDIAG		0x1c	/* Receive diagnostic */
#define SCSI_SDIAG		0x1d	/* Send diagnostic */
#define		SCSI_SDIAG_REINIT	0x60
#define		SCSI_SDIAG_DUMP_HW	0x61
#define		SCSI_SDIAG_DUMP_RAM	0x62
#define		SCSI_SDIAG_PATCH_HW	0x63
#define		SCSI_SDIAG_PATCH_RAM	0x64
#define		SCSI_SDIAG_SET_RD_ERR	0x65
#define			SCSI_SDIAG_RD_ERR_DEF	0x00
#define			SCSI_SDIAG_RD_ERR_NOC	0x01
#define			SCSI_SDIAG_RD_ERR_RPT	0x02

/* Class 01 commands */
#define SCSI_READC		0x25	/* Read capacity */
#define		SCSI_FULL_CAP		0x00
#define		SCSI_PART_CAP		0x01
#define SCSI_READ_DEFECTS       0x37


/*
 * Unsupported SCSI commands.
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
 * Tape commands.
 */
#define SCSI_REWIND		0x01	/* Rewind command */
#define SCSI_RETENTION		0x02	/* Retention a tape */
#define SCSI_WFM		0x10	/* Write a file mark */
#define SCSI_SPACE		0x11	/* Space (default blocks) fwd */
#define 	SCSI_SPACE_BLOCKS	0x00
#define 	SCSI_SPACE_FILEMARKS	0x01
#define 	SCSI_SPACE_SFILEMARKS	0x02
#define 	SCSI_SPACE_ENDOFDATA	0x03
#define         SCSI_SPACE_CODE         0x03
#define SCSI_ERASE		0x19	/* Erase a tape */
#define 	SCSI_ERASE_LONG		0x01
#define SCSI_LOAD_UNLOAD 	0x1b	/* Position media for load/unload */
#define 	SCSI_LOAD_MEDIA 	0x01
#define 	SCSI_RETEN_MEDIA 	0x02
#define SCSI_VARIABLE_BLOCKS    0x00    /* Qualifier for read and write */
#define SCSI_IMMEDIATE          0x01    /* Return prior to completion of cmd */
#define	SCSI_FIXED_BLOCKS	0x01	/* Qualifier for read and write */

/*
 * Sizes of data transferred for some standard commands.
 */
#define SIZE_CAP		8	/* #bytes in Read Capacity input data */
#define SIZE_TRANS		8	/* #bytes in Translate input data */
#define SIZE_INQ		4	/* #bytes in Inquiry output data */
#define SIZE_INQ_XTND		36	/* #bytes in extended Inquiry data */
#define SIZE_BDESC		12	/* #bytes in SCSI_MODES block descr. */
#define SIZE_MAXDATA		36	/* #bytes in largest data transfer */

#define SCSI_CMD6SZ		6	/* Scsi command length */
#define SCSI_CMD10SZ		10	/* Scsi command length */
#define SCSI_CMD12SZ		12	/* Scsi command length */

#define SCSI_MAXCMDSZ           12      /* Maximum SCSI command length */
/*
 * Structure for SCSI mode select command  (non-CCS devices)
 */
struct  scsi_modes {
			/* Command list */
	u_char	m_type;			/* Command type */
	u_char	m_unit;			/* Upper 3 bits are unit */
	u_char	m_pad1[2];		/* Reserved */
	u_char	m_ilen;			/* Length of info passed */
	u_char	m_cont;			/* Control byte */
			/* Parameter list */
	u_char	m_pad2[3];		/* Reserved */
	u_char	m_dlen;			/* Length of descript list */
			/* Extent descripter list */
	u_char	m_density;		/* Density code */
	u_char	m_pad3[4];		/* Reserved */
	u_char	m_bsize[3];		/* Block size */
			/* Drive parameter list */
	u_char	m_fcode;		/* Format code */
	u_char	m_cyls[2];		/* Cylinder count */
	u_char	m_heads;		/* Data head count */
	u_char	m_rwcc[2];		/* Reduced write current cylinder */
	u_char	m_wpc[2];		/* Write precompensation cylinder */
	char	m_lzone;		/* Landing zone position */
	u_char	m_srate;		/* Step pulse output rate code */
};

#define SCSI_MODES_HLEN		4	/* Size of command header */
#define SCSI_MODES_PLEN		10	/* Size of command and param list */
#define SCSI_MODES_DLEN		8	/* Length of extent decriptor list */
#define SCSI_MODES_ILEN		22	/* Bytes in data block */

/* 
 * Structures for SCSI format command (non-CCS devices).
 */
struct dlist {				/* Defect list entries */
	u_char	d_cyls[3];		/* Cyl of defect */
	u_char	d_heads;		/* Head number */
	u_char	d_bytes[4];		/* Bytes from index */
};

#define FORMAT_BUF		1024	/* Max bytes for Format Data */
#define MAX_DEFECTS		(FORMAT_BUF / sizeof(struct dlist))

struct	scsi_fmt {
			/* Command list */
	u_char	f_type;			/* Command type */
	u_char	f_misc;			/* 3 bit unit, data flag, 
					 * Complete list bits */
	u_char	f_data;			/* Data pattern */
	u_char	f_ileave[2];		/* Interleave */
	u_char	f_pad1;			/* Reserved */
			/* Defect list */
	u_char	f_full;			/* Full or cyl flag: cyl not on 4000 */
	u_char	f_spares;		/* Number spares/cyl: not on 4000 */
	u_char	f_dlen[2];		/* Length of defect list blocks */
	struct dlist dlist[MAX_DEFECTS];/* Blocks of defect list */
};

/*
 * scsi_fmt.f_misc flags.
 */
#define FMT_BBL_DATA		0x10	/* Bbl data exists */
#define FMT_CMPLT		0x08	/* Bbl data is complete */
#define FMT_USER_FMT		0x04	/* Use user-supplied fmt data */
#define FMT_DATA		0x02	/* Use user-supplied data pattern */
#define FMT_ALL		(FMT_BBL_DATA | FMT_CMPLT | FMT_USER_FMT | FMT_DATA)

/*
 * scsi_fmt.f_data.
 */
#define FMT_PAT			0x6D	/* Worst winchester data */

/*
 * scsi_fmt.f_full flags.
 */
#define	FMT_FULL		0x00	/* Complete drive */
#define	FMT_CYL			0x01	/* Single cylinder: not on 4000 */

/*
 * scsi_fmt.f_code.
 */
#define FMT_FCODE		0x01	/* Must be 1 */

/*
 * Other mode selection definitions for
 * sequential access devices.
 */
/* For the buffered mode field of the header */
#define MSEL_BFM_SYNC		0x00	/* Report write completions after
					 * its termination. */
#define MSEL_BFM_ASYNC		0x10	/* Report write completions once
					 * its data has been copied. */

/* For the speed field of the header */
#define MSEL_SPD_DEFAULT 	0x00	/* Use peripheral's default speed */
#define MSEL_SPD_LOWEST 	0x01	/* Use peripheral's lowest speed */


#define MSENSE_WP               0x80    /* Media is protected against writes */

/* Density codes for the 1st block descriptor */
#define MSEL_DEN_DEFAULT	0x00	/* Use peripheral's default density */
#define MSEL_DEN_X322		0x01	/* 800 CPI, NRZI */
#define MSEL_DEN_X339		0x02	/* 1600 CPI, PE */
#define MSEL_DEN_X354		0x03	/* 6250 CPI, GCR */
#define MSEL_DEN_QIC11		0x04	/* 1/4 inch, QIC-11 format */
#define MSEL_DEN_QIC24		0x05	/* 1/4 inch, QIC-24 format */
#define MSEL_DEN_X385		0x06	/* 3200 CPI, PE */

/*
 * A SCSI device number is derived from a
 * target adapter #, logical unit # pair
 * (both in the range 0 thru 7).
 */
#define SCSI_NDEVS	64 		/* Max units per SCSI bus (64) */
#define SCSI_UNIT(x)	((x) & 7)	/* Logical unit # from SCSI address */
#define SCSI_TARGET(x)	(((x) >> 3) & 7)/* Target adapter # from SCSI address */
#define SCSI_DEVNO(targ,lun)	(((targ) << 3) | lun)
					/* Device address on the SCSI bus */
#define SCSI_LUNSHFT	5
					/* For shifting logical unit numbers
					 * to put into a SCSI command. */
#define SCSI_MAX_LUN    7               /* Maximum SCSI logical unit number */
#define SCSI_MAX_TARGET 7               /* Maximum SCSI target adapter number */

/*
 * SCSI command termination status byte macros.
 */
#define STERM_CODE(x)           ((x) & 0x1E)

#define STERM_GOOD         0x00
#define STERM_CHECK_CONDITION   0x02
#define STERM_CONDITION_MET     0x04
#define STERM_BUSY              0x08
#define STERM_INTERMEDIATE      0x10
#define STERM_INTERMEDIATE_COND 0x12
#define STERM_RES_CONFLICT      0x18


#define SCSI_GOOD(status) (((status) & 0x1E) == 0)
#define SCSI_CHECK_CONDITION(status) (((status) & 0x1E) == 0x2)
#define	SCSI_CONDITION_MET(status) (((status) & 0x1E) == 0x4)
#define	SCSI_BUSY(status)	(((status) & 0x1E) == 0x8)
#define SCSI_INTERMEDIATE(status)	(((status) & 0x1E) == 0x10)
#define	SCSI_RES_CONFLICT(status)	(((status) & 0x1E) == 0x18)	

#define INQ_VEND	8
#define INQ_PROD	16
#define INQ_REV		4

/*
 * Inquiry command returned data.
 */
struct scinq {
	u_char	sc_devtype;		/* Type scsi device */
	u_char	sc_qualif;		/* Dev type qualifier */
	u_char	sc_version;		/* SCSI spec version */
	u_char	sc_reserved;
	u_char	sc_vlength;		/* Length of vendor unique data */
	u_char	sc_pad[3];
	char	sc_vendor[INQ_VEND];	/* name of the vendor */
	char	sc_product[INQ_PROD];	/* product ID */
	char	sc_revision[INQ_REV];	/* product fw rev level */
};

#define	INQ_LEN		sizeof (struct scinq)
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
	u_char	rs_key;			/* Filemark, EOM, ILI, and Sense Key */
	u_char	rs_info[4];		/* Information bytes, kludged to 
					 * preserve alignment */
	u_char	rs_addlen;		/* Additional length in bytes */
};

/*
 * Defines for scrsense.rs_error.
 */
#define	RS_VALID		0x80	/* Bit indicates error code is valid */
#define RS_ERRCLASS             0x70    /* Mask for error class */
#define	RS_SENSECODE		0x0f	/* Mask for error code */
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
#define	RS_ABORT		0x0b	/* Command aborted */
#define	RS_EQUAL		0x0c	/* Search data found equal comparison */
#define	RS_OVFLOW		0x0d	/* Volume overflow */
#define	RS_MISCMP		0x0e	/* Source and media data mis-compare */
#define	RS_RESKEY		0x0f	/* Reserved for future use */

/*
 * Read Capacity command returned data.
 */
struct wdcap {
				/* Highest addressable block on disk: */
	u_char cap_nblocks0;	/* MSB */
	u_char cap_nblocks1;
	u_char cap_nblocks2;
	u_char cap_nblocks3;	/* LSB */
				/* Formatted size of disk blocks: */
	u_char cap_bsize0;	/* MSB */
	u_char cap_bsize1;
	u_char cap_bsize2;
	u_char cap_bsize3;	/* LSB */
};

/*
 * Macros for extacting frequently accessed information 
 * from SCSI request sense data fields.
 */

#define SCSI_RS_INFO_VALID(rsp)	(((struct scrsense *)rsp)->rs_error & RS_VALID)
#define SCSI_RS_ERR_CLASS(rsp)	\
		(((struct scrsense *)rsp)->rs_error & RS_ERRCLASS)
#define SCSI_RS_ERR_CODE(rsp)	\
		(((struct scrsense *)rsp)->rs_error & RS_SENSECODE)
#define SCSI_RS_EOM_SET(rsp)	(((struct scrsense *)rsp)->rs_key & RS_EOM)
#define SCSI_RS_FILEMARK_SET(rsp) \
		(((struct scrsense *)rsp)->rs_key & RS_FILEMARK)
#define SCSI_RS_ILI_SET(rsp)	(((struct scrsense *)rsp)->rs_key & RS_ILI)
#define SCSI_RS_SENSE_KEY(rsp)	\
		(((struct scrsense *)rsp)->rs_key & RS_KEY)
#define SCSI_RS_ADDLEN(rsp, rbuflen) min((rbuflen) - sizeof(struct scrsense), \
		(uint)((struct srcsense *)rsp)->rs_addlen);


/*
 * The following data structure collects generic 
 * information about a SCSI command into one place, 
 * which can be passed to and filled in by SCSI library 
 * functions. 
 */
struct scsi_cmd {
	u_char dir;			/* Direction of data xfer, if any */
	u_char clen;			/* Amount of cmd being used */
	u_char cmd[SCSI_MAXCMDSZ];	/* The SCSI command to execute */
};

typedef struct scsi_cmd scsicmd_t;

/*
 *  Definitions for scsi_cmd.dir
 */
#define SDIR_NONE		0	/* No data transfered */
#define SDIR_HTOD		1	/* From host memory to device */
#define SDIR_DTOH		2	/* From device to host memory */


/*
 * Structures for SCSI Mode Select and Mode Sense commands.
 * The data for both commands is a parameter list.  For a 
 * typical device it consists of a header followed by zero 
 * or more block descriptors, optionally followed by a vendor 
 * unique parameter list.
 */

/* Basic command structure */
struct scmode_cmd {
	u_char	m_cmd;			/* Command type */
	u_char	m_unit; 		/* Upper 3 bits are logical unit */
	u_char	m_pad1[2];		/* Reserved */
	u_char	m_plen; 		/* Parameter list length in bytes */
	u_char	m_cont; 		/* Control byte */
};

/* For scmode_cmd.m_unit; special flags for disks */
#define MODE_UN_SMP   	0x01		/* Save Mode Parameters */
#define MODE_UN_PF   	0x10		/* Page Format */
/*
 * Generic SCSI command termination values.
 */

#define SSTAT_OK        	0	/* Good termination */
#define SSTAT_NODEV     	1	/* Unrecognized device */
#define SSTAT_BUSERR    	2	/* SCSI Bus error or reset */
#define SSTAT_NOTARGET  	3	/* Target Adapter does not respond */
#define SSTAT_BUSYTARGET	4	/* Target adapter is busy */
#define SSTAT_BUSYLUN   	5	/* Logical unit is busy */
#define SSTAT_CCHECK    	6	/* Check condition occurred.  Current
					 * sense data in extended format */
#define SSTAT_DCHECK    	7	/* Check condition occurred.  Deferred
					 * sense data in extended format */
#define SSTAT_UCHECK    	8	/* Check condition occurred.  Sense
					 * data in unrecognized format */
#define SSTAT_RESERVED  	9	/* Reserved termination value */



#ifdef KERNEL

/*
 * The following are global interfaces from ssm_scsi.c.
 */
extern void init_ssm_scsi_dev();
extern u_long scb_buf_iovects();
extern int ssm_scsi_probe_cmd();

/*
 * SCSI status summary, error message, and command string tables.
 */
extern char *scsi_errors[];
extern int num_scsi_errors;
extern char *scsi_commands[];
extern int num_scsi_commands;
extern char *scsi_status_msg[];

/*
 * SCSI common command interfaces.
 */
extern scsicmd_t *scsi_mode_select_cmd();
extern scsicmd_t *scsi_mode_sense_cmd();
extern scsicmd_t *scsi_test_unit_ready_cmd();

#ifdef notyet
extern scsicmd_t *scsi_inquiry_cmd();
extern scsicmd_t *scsi_request_sense_cmd();
extern int scb_num_iovects();
extern scsicmd_t *scsi_prevent_allow_removal_cmd();
#endif /* notyet */

#ifdef notyet
/*
 * SCSI command interfaces for Direct Access devices.
 */
extern scsicmd_t *scsi_DA_format_unit_cmd();
extern scsicmd_t *scsi_DA_read_capacity_cmd();
extern scsicmd_t *scsi_DA_read_cmd();
extern scsicmd_t *scsi_DA_reassign_blocks_cmd();
extern scsicmd_t *scsi_DA_rezero_unit_cmd();
extern scsicmd_t *scsi_DA_write_cmd();
#endif /* notyet */
	
/*
 * SCSI command interfaces for Sequential Access devices.
 */
extern scsicmd_t *scsi_SA_load_unload_cmd();
extern scsicmd_t *scsi_SA_read_cmd();
extern scsicmd_t *scsi_SA_rewind_cmd();
extern scsicmd_t *scsi_SA_space_cmd();
extern scsicmd_t *scsi_SA_write_cmd();
extern scsicmd_t *scsi_SA_write_filemarks_cmd();
extern scsicmd_t *scsi_SA_erase_cmd();

#ifdef notyet
extern scsicmd_t *scsi_SA_read_blk_limits_cmd();
extern scsicmd_t *scsi_SA_read_reverse_cmd();
extern scsicmd_t *scsi_SA_verify_cmd();
#endif /* notyet */

/*
 * SCSI common interfaces for command termination.
 */
extern int scsi_status();
extern void scsi_cmd_dump();
extern unsigned scsi_rsense_string();
extern int scsi_rs_info_bytes();
extern int scsi_rs_lba();

#endif KERNEL


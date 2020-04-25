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

/* $Header: scsiformat.h 1.1 89/08/24 $ */

/*
 * scsiformat.h
 *	defines and data structures used by scsi subsystem
 *	of online formatter.
 */

/* $Log:	scsiformat.h,v $
 */


/*
 * Defines used internally by scsi formatter 
 */
#define SCSI_PASSDEFAULT	1	/* verify pass default */
#define SCSI_PASSMAX		20	/* max. verify passes */
/*
 * Functions which require OVERWRITE to be set when there's
 * a valid VTOC on the disk.
 */
#define SCSI_OVERWRITES		(FORMAT|VERIFY)

/*
 * Defines used in constructing various scsi commands
 */
#define SDF_FORMPG	0x10	/* format with P & G lists + data */
#define SDF_FORMPG_ND	0x00	/* format with P & G lists, no data */
#define SCSI_DHEADSIZE	4	/* size of read defect data header */
#define SCSI_PLIST	0x10	/* read plist (for read defect data) */
#define SCSI_GLIST	0x08	/* read glist (for read defect data) */
#define SCSI_LISTMASK	PLIST|GLIST
	
/*
 * macro used to create number used in bootstring
 */
#define SCSI_BOOTNUM(c, d)	(((c) << 9) | (d))

/*
 * structure used for table of possible -t (format type)
 * values
 */
struct scsi_ftype {
	char fstr[32];
	int fvalue;
};

/*
 * macros used in doing byte swapping.
 */
#define itob4(i, b) \
	b[3] = (u_char) i; \
	b[2] = (u_char) (i >> 8); \
	b[1] = (u_char) (i >> 16); \
	b[0] = (u_char) (i >> 24);
#define itob3(i, b) \
	b[2] = (u_char) i; \
	b[1] = (u_char) (i >> 8); \
	b[0] = (u_char) (i >> 16);


/*
 * Defines and structure used for writing diagnostic
 * tracks
 */
#define	DIAG_PAT_LEN	(512 - sizeof(u_int))

struct csd_db {
	u_char	csd_db_pattern[DIAG_PAT_LEN];	/* test pattern */
	u_int	csd_db_blkno;			/* block number */
};

/*
 * Worse case csd pattern: e739c
 */
#define	CSD_DIAG_PAT_0	0xe7	/* must be in byte 0 of the block */
#define	CSD_DIAG_PAT_1	0x39	/* must be in byte 1 of the block */
#define	CSD_DIAG_PAT_2	0xce	/* must be in byte 2 of the block */
#define	CSD_DIAG_PAT_3	0x73	/* must be in byte 3 of the block */
#define	CSD_DIAG_PAT_4	0x9c	/* must be in byte 4 of the block */
#define	CSD_DIAG_PAT_SIZE 	5	/* repeated pattern length */

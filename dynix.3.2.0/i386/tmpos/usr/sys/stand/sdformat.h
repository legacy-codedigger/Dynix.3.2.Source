
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

/* $Header: sdformat.h 1.4 90/09/13 $ */


#define MANUALLY	0
#define FILE		1

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
 * tasks that can be done by the formatter
 */

struct task_type {
	int tt_cmd;
	char *tt_desc;
};

/*
 * tasks performable by the formatter
 */

#define EXIT	0
#define FORMAT	1
#define ADDBAD	2
#define WRITEVTOC	3
#define DISPLAY	4

#define DFLT_RPM        3600

/*
 * Diagnostic Block Structure
 */
#define	CSD_DIAG_PAT_BYTES	(512 - sizeof(u_int))

struct csd_db {
	u_char	csd_db_pattern[CSD_DIAG_PAT_BYTES];	/* test pattern */
	u_int	csd_db_blkno;				/* block number */
};

/*
 * Worse case csd pattern: e739c
 */
#define	CSD_DIAG_PAT_0		0xe7	/* must be in byte 0 of the block */
#define	CSD_DIAG_PAT_1		0x39	/* must be in byte 1 of the block */
#define	CSD_DIAG_PAT_2		0xce	/* must be in byte 2 of the block */
#define	CSD_DIAG_PAT_3		0x73	/* must be in byte 3 of the block */
#define	CSD_DIAG_PAT_4		0x9c	/* must be in byte 4 of the block */
#define	CSD_DIAG_PAT_SIZE	5	/* repeated pattern is 5 bytes long */

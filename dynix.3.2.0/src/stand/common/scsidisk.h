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

#ident	"$Header: scsidisk.h 1.1 90/09/13 $"

/* 
 *$Log:	scsidisk.h,v $
 */

struct drive_type {
	
	char	*dt_diskname;	/* corresponds with disktab */
	char	*dt_vendor;	/* parts of ID string from INQUIRY cmd */
	char	*dt_product;
	struct	st dt_st;	/* disk geometry */
	struct partition dt_part;	/* VTOC partition info */
	u_char	dt_inqformat;	/* format of INQUIRY return data */
	u_char	dt_reasslen;	/* bytes of defect data for REASSIGN BLOCKS */
	u_char	dt_formcode;	/* how to default format, CDB byte 1 */
	u_char	dt_pagecode;	/* page code for error recovery page */
	u_char	dt_pf;		/* byte 1 of CDB for MODE SELECT cmd */
	u_char	dt_tinymodelen;	/* length of short MODE SELECT command */
	u_char	dt_modebits;	/* which error recovery bits are set */
};

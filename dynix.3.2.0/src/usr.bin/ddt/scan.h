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
 * scan.h: version 1.1 of 11/17/82
 * 
 *
 * @(#)scan.h	1.1	(National Semiconductor)	11/17/82
 */

/* Definitions for self-terminating scanning module */

#define ESC 		0x1b	/* ESC is a special character appears as '$' */
#define	SCAN_SIZE	200	/* maximum line length */

int	scaninit();		/* Initialize for reading */
int	scanread();		/* Read next character */
int	scanbackup();		/* Put back last character */
int	scanabort();		/* Abort current command */

extern	char	scanbuffer[SCAN_SIZE+1];	/* buffer holding chars */
extern	char	*scanreadptr;			/* current read pointer */
extern	char	*scanwriteptr;			/* current write pointer */
extern	char	(*scanroutine)();		/* input read routine */

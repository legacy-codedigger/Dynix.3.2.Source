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

/* $Id: iorec.h,v 1.1 88/09/02 11:48:06 ksb Exp $ */

#include <stdio.h>
#define NAMSIZ 76

struct iorec {
	char		*fileptr;	/* ptr to file window */
	long		lcount;		/* number of lines printed */
	long		llimit;		/* maximum number of text lines */
	FILE		*fbuf;		/* FILE ptr */
	struct iorec	*fchain;	/* chain to next file */
	long		*flev;		/* ptr to associated file variable */
	char		*pfname;	/* ptr to name of file */
	long		funit;		/* file status flags */
	long		size;		/* size of elements in the file */
	char		fname[NAMSIZ];	/* name of associated UNIX file */
	char		buf[BUFSIZ];	/* I/O buffer */
	char		window[1];	/* file window element */
};

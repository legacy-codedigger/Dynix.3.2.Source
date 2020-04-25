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

/* $Header: dbm.h 2.1 90/05/08 $ */

#define	PBLKSIZ	1024
#define	DBLKSIZ	4096
#define	BYTESIZ	8
#ifndef NULL
#define	NULL	((char *) 0)
#endif

long	bitno;
long	maxbno;
long	blkno;
long	hmask;

char	pagbuf[PBLKSIZ];
char	dirbuf[DBLKSIZ];

int	dirf;
int	pagf;
int	dbrdonly;

typedef	struct
{
	char	*dptr;
	int	dsize;
} datum;

datum	fetch();
datum	makdatum();
datum	firstkey();
datum	nextkey();
datum	firsthash();
long	calchash();
long	hashinc();


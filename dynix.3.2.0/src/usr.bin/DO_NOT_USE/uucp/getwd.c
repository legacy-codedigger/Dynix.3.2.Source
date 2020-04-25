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

#ifndef	lint
static char rcsid[] = "$Header: getwd.c 1.2 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)getwd.c	5.4 (Berkeley) 6/19/85";
#endif

#include "uucp.h"

/*
 *	get working directory
 *
 *	return codes  0 = FAIL
 *		      wkdir = SUCCES
 */

char *
getwd(wkdir)
register char *wkdir;
{
	register FILE *fp;
	extern FILE *rpopen();
	extern int rpclose();
	register char *c;

	*wkdir = '\0';
	if ((fp = rpopen("PATH=/bin:/usr/bin:/usr/ucb;pwd 2>&-", "r")) == NULL)
		return 0;
	if (fgets(wkdir, 100, fp) == NULL) {
		rpclose(fp);
		return 0;
	}
	if (*(c = wkdir + strlen(wkdir) - 1) == '\n')
		*c = '\0';
	rpclose(fp);
	return wkdir;
}

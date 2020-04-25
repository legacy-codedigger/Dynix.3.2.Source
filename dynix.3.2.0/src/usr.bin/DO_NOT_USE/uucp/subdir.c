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
static char rcsid[] = "$Header: subdir.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)subdir.c	5.4 (Berkeley) 6/23/85";
#endif

#include "uucp.h"

/*LINTLIBRARY*/

/*
 * By Tom Truscott, March 1983
 *
 * Prefix table.
 * If a prefix is "abc", for example,
 * then any file Spool/abc... is mapped to Spool/abc/abc... .
 * The first prefix found is used, so D.foo should preceed D. in table.
 *
 * Each prefix must be a subdirectory of Spool, owned by uucp!
 * Remember: use cron to uuclean these directories daily,
 * and check them manually every now and then.  Beware complacency!
 */

static char *dprefix[] = {
	DLocalX,	/* Outbound 'xqt' request files */
	DLocal,		/* Outbound data files */
	"D.",		/* Other "D." files (remember the "."!) */
	"C.",		/* "C." subdirectory */
	"X.",		/* "X." subdirectory */
	"TM.",		/* Temporaries for inbound files */
	0
};

/*
 * filename mapping kludges to put uucp work files in other directories.
 */

#define	BUFLEN	50

static	char fn1[BUFLEN], fn2[BUFLEN];	/* remapped filename areas */
static	int	inspool;		/* true iff working dir is Spool */

/*
 * return (possibly) remapped string s
 */
char *
subfile(as)
char *as;
{
	register char *s, **p;
	register int n;
	static char *tptr = NULL;

	/* Alternate buffers so "link(subfile(a), subfile(b))" works */
	if (tptr != fn1)
		tptr = fn1;
	else
		tptr = fn2;

	s = as;
	tptr[0] = '\0';

	/* if s begins with Spool/, copy that to tptr and advance s */
	if (strncmp(s, Spool, n = strlen(Spool)) == 0 && s[n] == '/') {
		if (!inspool) {
			strcpy(tptr, Spool);
			strcat(tptr, "/");
		}
		s += n + 1;
	}
	else
		if (!inspool)
			return as;

	/* look for first prefix which matches, and make subdirectory */
	for (p = &dprefix[0]; *p; p++) {
		if (strncmp(s, *p, n = strlen(*p))==0 && s[n] && s[n] != '/') {
			strcat(tptr, *p);
			strcat(tptr, "/");
			strcat(tptr, s);
			return tptr;
		}
	}
	return as;
}

/*
 * save away filename
 */
subchdir(s)
register char *s;
{
	inspool = (strcmp(s, Spool) == 0);
	return chdir(s);
}

/*
 * return possibly corrected directory for searching
 */
static char xdir[BUFSIZ];
char *
subdir(d, pre)
register char *d, pre;
{
	if (strcmp(d, Spool) == 0)
		if (pre == CMDPRE)
			return CMDSDIR;
		else if (pre == XQTPRE) {
			if (xdir[0] == '\0')
				sprintf(xdir,"%s/X.",Spool);
			return xdir;

		}
	return d;
}

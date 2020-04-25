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

/* $Header: getlogin.c 2.1 90/03/08 $ */

#include <utmp.h>

static	char UTMP[]	= "/etc/utmp";
static	struct utmp ubuf;

char *
getlogin()
{
	register int me, uf;
	register char *cp;

	if (!(me = ttyslot()))
		return(0);
	if ((uf = open(UTMP, 0)) < 0)
		return (0);
	lseek (uf, (long)(me*sizeof(ubuf)), 0);
	if (read(uf, (char *)&ubuf, sizeof (ubuf)) != sizeof (ubuf)) {
		close(uf);
		return (0);
	}
	close(uf);
	if (ubuf.ut_name[0] == '\0')
		return (0);
	ubuf.ut_name[sizeof (ubuf.ut_name)] = ' ';
	for (cp = ubuf.ut_name; *cp++ != ' '; )
		;
	*--cp = '\0';
	return (ubuf.ut_name);
}

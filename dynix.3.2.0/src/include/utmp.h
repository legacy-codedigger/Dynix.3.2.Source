/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/* $Header: utmp.h 2.9 91/01/23 $ */

#define _PATH_UTMP      "/etc/utmp"
#define _PATH_WTMP      "/usr/adm/wtmp"
#define _PATH_LASTLOG   "/usr/adm/lastlog"

#define UT_NAMESIZE     8
#define UT_LINESIZE     8
#define UT_HOSTSIZE     16

/*
 * Structure of utmp and wtmp files.
 *
 * Assuming the number 8 is unwise.
 */
struct utmp {
	char	ut_line[UT_LINESIZE];		/* tty name */
	char	ut_name[UT_NAMESIZE];		/* user id */
	char	ut_host[UT_HOSTSIZE];		/* host name, if remote */
	long	ut_time;			/* time on */
#ifdef SCGACCT
	char	ut_account;			/* account identifier */
#endif
};

/*
 * This is a utmp entry that does not correspond to a genuine user
 */
#define nonuser(ut) ((ut).ut_host[0] == '\0' && \
	ispseudotty((ut).ut_line))

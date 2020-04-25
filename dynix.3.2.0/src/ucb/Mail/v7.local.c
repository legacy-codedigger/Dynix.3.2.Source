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
static char *rcsid = "$Header: v7.local.c 2.1 87/04/02 $";
#endif

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)v7.local.c	5.2 (Berkeley) 6/21/85";
#endif not lint

/*
 * Mail -- a mail program
 *
 * Version 7
 *
 * Local routines that are installation dependent.
 */

#include "rcv.h"

/*
 * Locate the user's mailbox file (ie, the place where new, unread
 * mail is queued).  In Version 7, it is in /usr/spool/mail/name.
 */

findmail()
{
	register char *cp;

	cp = copy("/usr/spool/mail/", mailname);
	copy(myname, cp);
	if (isdir(mailname)) {
		stradd(mailname, '/');
		strcat(mailname, myname);
	}
}

/*
 * Get rid of the queued mail.
 */

demail()
{

	if (value("keep") != NOSTR)
		close(creat(mailname, 0666));
	else {
		if (remove(mailname) < 0)
			close(creat(mailname, 0666));
	}
}

/*
 * Discover user login name.
 */

username(uid, namebuf)
	char namebuf[];
{
	register char *np;

	if (uid == getuid() && (np = getenv("USER")) != NOSTR) {
		strncpy(namebuf, np, PATHSIZE);
		return(0);
	}
	return(getname(uid, namebuf));
}

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
static char rcsid[] = "$Header: getpwinfo.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)getpwinfo.c	5.3 (Berkeley) 6/19/85";
#endif

#include "uucp.h"
#include <pwd.h>

/*LINTLIBRARY*/

/*
 *	get passwd file info for uid
 *
 *	return codes:  SUCCESS  |  FAIL
 *
 */

guinfo(uid, name, path)
int uid;
register char *path, *name;
{
	register struct passwd *pwd;
	struct passwd *getpwuid(), *getpwnam();
	char *getlogin(), *getenv(), *l;

	if ((l = getlogin()) == NULL) {
		l = getenv("USER");
	}
	if (l != NULL) {
		pwd = getpwnam(l);
		if (pwd != NULL && pwd->pw_uid == uid)
			goto setup;
	}
	if ((pwd = getpwuid(uid)) == NULL) {
		/* can not find uid in passwd file */
		*name = '\0';
		*path = '\0';
		return FAIL;
	}

    setup:
	strcpy(path, pwd->pw_dir);
	strcpy(name, pwd->pw_name);
	return SUCCESS;
}


/*
 *	get passwd file info for name
 *
 *	return codes:  SUCCESS  |  FAIL
 */

gninfo(name, uid, path)
char *path, *name;
int *uid;
{
	register struct passwd *pwd;
	struct passwd *getpwnam();

	if ((pwd = getpwnam(name)) == NULL) {
		/* can not find name in passwd file */
		*path = '\0';
		return FAIL;
	}

	strcpy(path, pwd->pw_dir);
	*uid = pwd->pw_uid;
	return SUCCESS;
}

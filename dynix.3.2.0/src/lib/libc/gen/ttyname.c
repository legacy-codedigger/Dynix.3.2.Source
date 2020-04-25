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

/* $Header: ttyname.c 2.0 86/01/28 $
 *
 * ttyname(f): return "/dev/ttyXX" which the the name of the
 * tty belonging to file f.
 *  NULL if it is not a tty
 */

#define	NULL	0
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/stat.h>

static	char	dev[]	= "/dev/";
char	*strcpy();
char	*strcat();

char *
ttyname(f)
{
	struct stat fsb;
	struct stat tsb;
	register struct direct *db;
	register DIR *df;
	static char rbuf[32];

	if (isatty(f)==0)
		return(NULL);
	if (fstat(f, &fsb) < 0)
		return(NULL);
	if ((fsb.st_mode&S_IFMT) != S_IFCHR)
		return(NULL);
	if ((df = opendir(dev)) == NULL)
		return(NULL);
	while ((db = readdir(df)) != NULL) {
		if (db->d_ino != fsb.st_ino)
			continue;
		strcpy(rbuf, dev);
		strcat(rbuf, db->d_name);
		if (stat(rbuf, &tsb) < 0)
			continue;
		if (tsb.st_dev == fsb.st_dev && tsb.st_ino == fsb.st_ino) {
			closedir(df);
			return(rbuf);
		}
	}
	closedir(df);
	return(NULL);
}

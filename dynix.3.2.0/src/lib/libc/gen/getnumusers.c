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

/* $Header: getnumusers.c 1.2 89/07/27 $ */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <utmp.h>
#include <stdio.h>

extern	char *malloc();
#define	EQ(a,b)	   (a[0] && b[0] && strncmp(a, b, sizeof(a)) == 0)
#define	MATCH(c,d) (EQ((c)->ut_name, (d)->ut_name) && \
		    EQ((c)->ut_host, (d)->ut_host))

/*
 * getnumusers
 *	Calculate the number of logged in users
 *	possibly including new utmp entry
 *
 *	Counting method is:
 *	each real login counts once.
 *	the first user/host rlogin counts once.
 *	additional user/host rlogins don't count.
 *	root logins beyond the usrlimit don't count (root can always log in)
 */
getnumusers(fd, up)
	int	fd;
	struct	utmp *up;
{
	register struct utmp *p, *u, *v;
	register int n, fsize, rootcount;
	struct	 stat stbuf;

	if (fstat(fd, &stbuf) != 0)
		return(-1);
	fsize = stbuf.st_size;
	if ((p = (struct utmp *)malloc((unsigned)fsize)) == NULL)
		return(-1);
	lseek(fd, (off_t)0, 0);
	if (read(fd, (char *)p, fsize) != fsize) {
		free((char *) p);
		return(-1);
	}
	lseek(fd, (off_t)0, 0);
	n = 0;
	rootcount = 0;
	for (u = p; u < &p[fsize/sizeof(*p)]; u++) {
		if (u->ut_name[0] == '\0')
			continue;
		if (u->ut_host[0] != '\0') {
			for (v = p; v < u; v++) {
				if (MATCH(v, u)) {
					--n;
					break;
				}
			}
		}
		++n;
		if (EQ(u->ut_name, "root")) {
			for (v = p; v < u; v++) {
				if (MATCH(v, u)) {
					--rootcount;
					break;
				}
			}
			++rootcount;
		}
	}
	/* root logins above the limit don't count */
	if (n > getmaxusers()) {
		n = MAX(getmaxusers(), n - rootcount);
	}
	/*
	 * Add new user to count if not duplicate
	 */
	rootcount = 0;
	if (up != NULL) {
		if (up->ut_host[0] != '\0') {
			for (u = p; u < &p[fsize/sizeof(*p)]; u++) {
				if (MATCH(u, up)) {
					--n;
					break;
				}
			}
		}
		++n;
		if (EQ(up->ut_name, "root")) {
			++rootcount;
		}
	}
	if (n > getmaxusers()) {
		n = MAX(getmaxusers(), n - rootcount);
	}
	free((char *) p);
	return (n);
}

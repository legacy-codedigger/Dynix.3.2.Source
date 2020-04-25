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

#ifndef lint
static char rcsid[] = "$Header: start.c 2.0 86/01/28 $";
#endif

#include "stdio.h"
#include "lrnref.h"
#include <sys/types.h>
#include <sys/dir.h>
#define	ND	64

start(lesson)
char *lesson;
{
	struct direct dv[ND], *dm, *dp;
	int f, c, n;
	char where [100];

	f = open(".", 0);		/* clean up play directory */
	n = read(f, dv, ND*sizeof(*dp));
	n /= sizeof(*dp);
	if (n==ND)
		fprintf(stderr, "Start:  play directory too long\n");
	dm = dv+n;
	for(dp=dv; dp<dm; dp++)
		if (dp->d_ino) {
			n = strlen(dp->d_name);
			if (dp->d_name[n-2] == '.' && dp->d_name[n-1] == 'c')
				continue;
			c = dp->d_name[0];
			if (c>='a' && c<= 'z')
				unlink(dp->d_name);
		}
	close(f);
	if (ask)
		return;
	sprintf(where, "%s/%s/L%s", direct, sname, lesson);
	if (access(where, 04)==0)	/* there is a file */
		return;
	perror(where);
	fprintf(stderr, "Start:  no lesson %s\n",lesson);
	wrapup(1);
}

fcopy(new,old)
char *new, *old;
{
	char b[BUFSIZ];
	int n, fn, fo;
	fn = creat(new, 0666);
	fo = open(old,0);
	if (fo<0) return;
	if (fn<0) return;
	while ( (n=read(fo, b, BUFSIZ)) > 0)
		write(fn, b, n);
	close(fn);
	close(fo);
}

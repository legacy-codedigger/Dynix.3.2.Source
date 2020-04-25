/* @(#)$Copyright:	$
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
static char *rcsid = "$Header: sendfile.c 1.3 84/12/18 $";
#endif

#include <stdio.h>
#include "host.h"

/* send a file */
sendfile(s)
char *s;
{
	register FILE *fd;
	register int ccount;
	register int lines = 0, count = 0;
	char cc[1024];
	char lbuf[50];

	fd = fopen(s, "r");		/* open the file */
	if (fd < 0) {
		printf("%s: file %s not found\n", myname, s);
		goto err;
	}
	while (fgets(cc, 1024, fd) != NULL) {
		ccount = 0;
		while (cc[ccount++])
			;
		(void)write(port, cc, ccount);
		lines++;
		while (count--)
			printf("\b \b");
		(void)sprintf(lbuf, "%d", lines);
		count = strlen(lbuf);
		printf("%s", lbuf);
		(void)fflush(stdout);
	}
	fclose(fd);
err:
	fflush(stdout);
}

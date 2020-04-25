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
static char rcsid[] = "$Header: necf.c 2.0 86/01/28 $";
#endif

#include <stdio.h>
#include <sgtty.h>

#define PAGESIZE	66

main()
{
	extern char _sobuf[BUFSIZ];
	extern char *rindex();
	char line[256];
	register char c, *cp;
	register lnumber;

	setbuf(stdout, _sobuf);
#ifdef SHEETFEEDER
	printf("\033=\033\033\033O\f");
#else
	printf("\033=");
#endif
	lnumber = 0;
	while (fgets(line, sizeof(line), stdin) != NULL) {
#ifdef SHEETFEEDER
		if (lnumber == PAGESIZE-1) {
			putchar('\f');
			lnumber = 0;
		}
		if (lnumber >= 2) {
#endif
#ifdef TTY
			if ((cp = rindex(line, '\n')) != NULL)
				*cp = '\r';
#endif
			printf("%s", line);
#ifdef SHEETFEEDER
		}
		lnumber++;
#endif
	}
	fflush (stdout);
}

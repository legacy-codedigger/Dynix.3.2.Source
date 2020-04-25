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
static char rcsid[] = "$Header: calendar.c 2.1 86/06/04 $";
static	char *sccsid = "@(#)calendar.c	4.5 (Berkeley) 84/05/07";
#endif

/* /usr/lib/calendar produces an egrep -f file
   that will select today's and tomorrow's
   calendar entries, with special weekend provisions

   used by calendar command
*/
#include <sys/time.h>

#define DAY (3600*24L)

char *month[] = {
	"[Jj]an",
	"[Ff]eb",
	"[Mm]ar",
	"[Aa]pr",
	"[Mm]ay",
	"[Jj]un",
	"[Jj]ul",
	"[Aa]ug",
	"[Ss]ep",
	"[Oo]ct",
	"[Nn]ov",
	"[Dd]ec"
};
struct tm *localtime();

tprint(t)
long t;
{
	struct tm *tm;
	tm = localtime(&t);
	printf("(^|[ 	(,;])((%s[^ \t]*[ \t]*|(0%d|%d)/)0*%d)([^0123456789]|$)\n",
		month[tm->tm_mon],
		tm->tm_mon + 1, tm->tm_mon + 1, tm->tm_mday);
	printf("(^|[ 	(,;])((\\*[ \t]*)0*%d)([^0123456789]|$)\n",
		tm->tm_mday);
}

main()
{
	long t;
	time(&t);
	tprint(t);
	switch(localtime(&t)->tm_wday) {
	case 5:
		t += DAY;
		tprint(t);
	case 6:
		t += DAY;
		tprint(t);
	default:
		t += DAY;
		tprint(t);
	}
}

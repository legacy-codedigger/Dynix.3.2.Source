/* $Copyright: $
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

#ifndef lint
static char rcsid[] = "$Header: hunt8.c 2.1 1991/05/20 00:16:00 $";
#endif

#include <stdio.h>
#include <assert.h>
#include "pathnames.h"
#define unopen(fil) {if (fil!=NULL) {fclose(fil); fil=NULL;}}

extern long indexdate, gdate();
extern FILE *iopen();
runbib (s)
char *s;
{
	/* make a file suitable for fgrep */
	char tmp[200];
	sprintf(tmp, "%s '%s' > '%s.ig'", _PATH_MKEY,s,s);
	system(tmp);
}

makefgrep(indexname)
char *indexname;
{
	FILE *fa, *fb;
	if (ckexist(indexname, ".ig"))
	{
		/* existing gfrep -type index */
# if D1
		fprintf(stderr, "found fgrep\n");
# endif
		fa = iopen(indexname, ".ig");
		fb = iopen(indexname, "");
		if (gdate(fb)>gdate(fa))
		{
			if (fa!=NULL)
				fclose(fa);
			runbib(indexname);
			fa= iopen(indexname, ".ig");
		}
		indexdate = gdate(fa);
		unopen(fa); 
		unopen(fb);
	}
	else
		if (ckexist(indexname, ""))
		{
			/* make fgrep */
# if D1
			fprintf(stderr, "make fgrep\n");
# endif
			runbib(indexname);
			time(&indexdate);
		}
		else /* failure */
		return(0);
	return(1); /* success */
}

ckexist(s, t)
char *s, *t;
{
	char fnam[100];
	strcpy (fnam, s);
	strcat (fnam, t);
	return (access(fnam, 04) != -1);
}

FILE *
iopen(s, t)
char *s, *t;
{
	char fnam[100];
	FILE *f;
	strcpy (fnam, s);
	strcat (fnam, t);
	f = fopen (fnam, "r");
	if (f == NULL)
	{
		err("Missing expected file %s", fnam);
		exit(1);
	}
	return(f);
}

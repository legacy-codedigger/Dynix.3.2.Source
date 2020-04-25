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
static char rcsid[] = "$Header: util.c 1.1 86/12/18 $";
#endif

#ifndef lint
/* @(#)util.c	2.1 86/04/16 NFSSRC */ 
static  char sccsid[] = "@(#)util.c 1.1 86/02/05 (C) 1985 Sun Microsystems, Inc.";
#endif

#include <stdio.h>
#include "util.h"




/*
 * This is just like fgets, but recognizes that "\\n" signals a continuation
 * of a line
 */
char *
getline(line,maxlen,fp)
	char *line;
	int maxlen;
	FILE *fp;
{
	register char *p;
	register char *start;


	start = line;

nextline:
	if (fgets(start,maxlen,fp) == NULL) {
		return(NULL);
	}	
	for (p = start; ; p++) {
		if (*p == '\n') {       
			if (*(p-1) == '\\') {
				start = p - 1;
				goto nextline;
			} else {
				return(line);	
			}
		}
	}
}	




	
void
fatal(message)
	char *message;
{
	(void) fprintf(stderr,"fatal error: %s\n",message);
	exit(1);
}

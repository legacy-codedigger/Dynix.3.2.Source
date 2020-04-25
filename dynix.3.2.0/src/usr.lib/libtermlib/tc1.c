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
static char rcsid[] = "$Header: tc1.c 2.0 86/01/28 $";
#endif

/*
 * tc1 [term]
 * dummy program to test termlib.
 * gets entry, counts it, and prints it.
 */
#include <stdio.h>
char buf[1024];
char *getenv();

main(argc, argv) char **argv; {
	char *p;
	int rc;

	if (argc < 2)
		p = getenv("TERM");
	else
		p = argv[1];
	rc = tgetent(buf,p);
	printf("tgetent returns %d, len=%d, text=\n'%s'\n",rc,strlen(buf),buf);
}

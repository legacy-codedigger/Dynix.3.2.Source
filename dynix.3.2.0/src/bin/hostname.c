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
static char rcsid[] = "$Header: hostname.c 2.0 86/01/28 $";
#endif

/*
 * hostname -- get (or set hostname)
 */
#include <stdio.h>

char hostname[32];
extern int errno;

main(argc,argv)
	char *argv[];
{
	int	myerrno;

	argc--;
	argv++;
	if (argc) {
		if (sethostname(*argv,strlen(*argv)))
			perror("sethostname");
		myerrno = errno;
	} else {
		gethostname(hostname,sizeof(hostname));
		myerrno = errno;
		printf("%s\n",hostname);
	}
	exit(myerrno);
}

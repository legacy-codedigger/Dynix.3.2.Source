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
static char rcsid[] = "$Header: domainname.c 1.2 86/12/21 $";
#endif

#ifndef lint
static	char sccsid[] = "@(#)domainname.c 1.1 85/05/30 Copyr 1984 Sun Micro";
#endif
/*
 * domainname -- get (or set domainname)
 */
#include <stdio.h>

char domainname[256];
extern int errno;

main(argc,argv)
	char *argv[];
{
	int	myerrno;

	argc--;
	argv++;
	if (argc) {
		if (setdomainname(*argv,strlen(*argv)))
			perror("setdomainname");
		myerrno = errno;
	} else {
		getdomainname(domainname,sizeof(domainname));
		myerrno = errno;
		printf("%s\n",domainname);
	}
	exit(myerrno);
}

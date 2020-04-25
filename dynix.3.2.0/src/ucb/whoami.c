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
static char rcsid[] = "$Header: whoami.c 2.0 86/01/28 $";
#endif

#include <pwd.h>
/*
 * whoami
 */
struct	passwd *getpwuid();

main()
{
	register struct passwd *pp;

	pp = getpwuid(getuid());
	if (pp == 0) {
		printf("Intruder alert.\n");
		exit(1);
	}
	printf("%s\n", pp->pw_name);
	exit(0);
}

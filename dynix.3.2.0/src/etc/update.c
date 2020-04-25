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
static char rcsid[] = "$Header: update.c 2.0 86/01/28 $";
#endif

/*
 * Update the file system every 30 seconds.
 * For cache benefit, open certain system directories.
 */

#include <signal.h>

char *fillst[] = {
	"/bin",
	"/lib",
	"/usr",
	"/usr/bin",
	"/usr/lib",
	"/usr/ucb",
	0,
};

main()
{
	char **f;

	if(fork())
		exit(0);
	close(0);
	close(1);
	close(2);
	for(f = fillst; *f; f++)
		open(*f, 0);
	dosync();
	for(;;)
		pause();
}

dosync()
{
	sync();
	signal(SIGALRM, dosync);
	alarm(30);
}

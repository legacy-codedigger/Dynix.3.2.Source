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
static char rcsid[] = "$Header: maktee.c 2.0 86/01/28 $";
#endif

#include "stdio.h"
#include "signal.h"
#include "lrnref.h"

static int oldout;
static char tee[50];

maktee()
{
	int fpip[2], in, out;

	if (tee[0] == 0)
		sprintf(tee, "%s/bin/lrntee", direct);
	pipe(fpip);
	in = fpip[0];
	out= fpip[1];
	if (fork() == 0) {
		signal(SIGINT, SIG_IGN);
		close(0);
		close(out);
		dup(in);
		close(in);
		execl (tee, "lrntee", 0);
		perror(tee);
		fprintf(stderr, "Maktee:  lrntee exec failed\n");
		exit(1);
	}
	close(in);
	fflush(stdout);
	oldout = dup(1);
	close(1);
	if (dup(out) != 1) {
		perror("dup");
		fprintf(stderr, "Maktee:  error making tee for copyout\n");
	}
	close(out);
	return(1);
}

untee()
{
	int x;

	fflush(stdout);
	close(1);
	dup(oldout);
	close(oldout);
	wait(&x);
}

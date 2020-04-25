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
static char rcsid[] = "$Header: hunt.c 2.0 86/01/28 $";
#endif

#include "tip.h"

extern char *getremote();
extern char *rindex();

static	jmp_buf deadline;
static	int deadfl;

dead()
{

	deadfl = 1;
	longjmp(deadline, 1);
}

hunt(name)
	char *name;
{
	register char *cp;
	int (*f)();

	f = signal(SIGALRM, dead);
	deadfl = 0;
	while (cp = getremote(name)) {
		uucplock = rindex(cp, '/')+1;
		if (mlock(uucplock) < 0) {
			delock(uucplock);
			continue;
		}
		/*
		 * Straight through call units, such as the BIZCOMP,
		 * VADIC and the DF, must indicate they're hardwired in
		 *  order to get an open file descriptor placed in FD.
		 * Otherwise, as for a DN-11, the open will have to
		 *  be done in the "open" routine.
		 */
		if (!HW)
			break;
		if (setjmp(deadline) == 0) {
			deadfl = 0;		/* going for line */
			alarm(10);
			FD = open(cp, O_RDWR);
			if (FD < 0) {
				char buf[128];
				extern int errno;

				if (errno == EBUSY)
					fprintf(stderr, "tip: line %.20s open by another process with exclusive use set\n", cp);
				else {
					sprintf(buf, "tip: line %.20s", cp);
					perror(buf);
				}
			}
		}
		alarm(0);
		if (!deadfl && FD > 0) {
			ioctl(FD, TIOCEXCL, 0);
			ioctl(FD, TIOCHPCL, 0);
			signal(SIGALRM, SIG_DFL);
			return ((int)cp);
		}
		delock(uucplock);
	}
	signal(SIGALRM, f);
	return (deadfl ? -1 : (int)cp);
}

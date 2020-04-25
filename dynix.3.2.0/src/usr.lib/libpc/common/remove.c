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

/* $Header: remove.c 1.1 89/03/12 $ */
#include "h00vars.h"

REMOVE(name, namlim)
char *name;
long namlim;
{
	register int	cnt;
	register int	maxnamlen = namlim;
	char		namebuf[NAMSIZ];

	/*
	 * trim trailing blanks, and insure that the name 
	 * will fit into the file structure
	 */
	for (cnt = 0; cnt < maxnamlen; )
		if (name[cnt] == '\0' || name[cnt++] == ' ')
			break;
	if (cnt >= NAMSIZ) {
		ERROR("%s: File name too long\n", name);
		/*NOTREACHED*/
		return;
	}
	maxnamlen = cnt;

	/*
	 * put the name into the buffer with null termination
	 */
	for (cnt = 0; cnt < maxnamlen; cnt++)
		namebuf[cnt] = name[cnt];
	namebuf[cnt] = '\0';
	if (unlink(namebuf)) {
		PERROR("Could not remove ", namebuf);
		/*NOTREACHED*/
	}
}

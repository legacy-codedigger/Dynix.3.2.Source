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

/* $Header: reade.c 1.1 89/03/12 $ */
#include "h00vars.h"

long
READE(curfile, name)
register IOREC *curfile;
char *name;
{
	register short	*sptr;
	register int	len;
	register int	nextlen;
	register int	cnt;
	register int	retval;
	char		*cp;
	static char	namebuf[NAMSIZ];

	if (curfile->funit & FWRITE) {
		ERROR("%s: Attempt to read, but open for writing\n", curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	UNSYNC(curfile);
	retval = fscanf(curfile->fbuf,
	    "%*[ \t\n]%74[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789]",
	    namebuf);
	if (retval == EOF) {
		ERROR("%s: Tried to read past end of file\n", curfile->pfname);
		/*NOTREACHED*/
		return;
	}
	if (retval == 0)
		goto ename;
	curfile->funit &= ~EOLN;
	curfile->funit |= SYNC;
	for (len = 0; len < NAMSIZ && namebuf[len]; len++)
		/* empty */;
	len++;
	sptr = (short *)name;
	cnt = *sptr++;
	cp = name + sizeof(short) + *sptr;
	do	{
		nextlen = *sptr++;
		nextlen = *sptr - nextlen;
		if (nextlen == len && RELEQ(len, namebuf, cp)) {
			return *((short *) name) - cnt;
		}
		cp += (int)nextlen;
	} while (--cnt);
ename:
	ERROR("Unknown name \"%s\" found on enumerated type read\n", namebuf);
	/*NOTREACHED*/
}

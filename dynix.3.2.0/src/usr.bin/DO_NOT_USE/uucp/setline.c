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
static char rcsid[] = "$Header: setline.c 2.1 87/05/27 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)setline.c	5.3 (Berkeley) 6/20/85";
#endif

#include "uucp.h"
#ifdef	USG
#include <termio.h>
#endif

#define PACKSIZE	64
#define SNDFILE	'S'
#define RCVFILE 'R'
#define RESET	'X'

/*LINTLIBRARY*/

/*
 *	optimize line setting for sending or receiving files
 *
 *	return code - none
 */
/*ARGSUSED*/
setupline(type)
char type;
{
#ifdef	USG
	static struct termio tbuf, sbuf;
	static int set = 0;

	DEBUG(2, "setline - %c\n", type);
	if (IsTcpIp)
		return;
	switch(type) {
	case SNDFILE:
		break;
	case RCVFILE:
		ioctl(Ifn, TCGETA, &tbuf);
		sbuf = tbuf;
		tbuf.c_cc[VMIN] = PACKSIZE;
		ioctl(Ifn, TCSETAW, &tbuf);
		set++;
		break;
	case RESET:
		if (set == 0) break;
		set = 0;
		ioctl(Ifn, TCSETAW, &sbuf);
		break;
	}
#endif
}

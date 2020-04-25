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

/* $Header: ttyinit.c 2.0 86/01/28 $
 *
 * sgtty stuff
 */

#include <sgtty.h>

struct	sgttyb	_ttyb;
struct	tchars	_otch, _ntch;
int	_normf;

/*
 * Routines for dealing with the unix tty modes
 */

#include "2648.h"

ttyinit()
{
	if (strcmp(getenv("TERM"), "hp2648") == 0)
		_on2648 = 1;
	ioctl(fileno(stdin), TIOCGETP, &_ttyb);
	ioctl(fileno(stdin), TIOCGETC, &_otch);
	_ntch = _otch;
	_ntch.t_quitc = _ntch.t_startc = _ntch.t_stopc = -1;
	_normf = _ttyb.sg_flags;
	_ttyb.sg_flags |= CBREAK;
	_ttyb.sg_flags &= ~(ECHO|CRMOD);
	ioctl(fileno(stdin), TIOCSETN, &_ttyb);
	ioctl(fileno(stdin), TIOCSETC, &_ntch);
	gdefault();
	zoomn(1);
	zoomon();
	kon();
	rboff();
	_cursoron = 1;	/* to force it off */
	_escmode = NONE;
	curoff();
	clearg();
	gon();
	aoff();
}

done()
{
	goff();
	koff();
	aon();
	sync();
	escseq(NONE);
	lowleft();
	printf("\n");
	fflush(stdout);
	_ttyb.sg_flags = _normf;
	ioctl(fileno(stdin), TIOCSETN, &_ttyb);
	ioctl(fileno(stdin), TIOCSETC, &_otch);
}

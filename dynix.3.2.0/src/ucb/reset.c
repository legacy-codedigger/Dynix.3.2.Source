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
static char rcsid[] = "$Header: reset.c 2.0 86/01/28 $";
#endif

/*
 * reset - restore tty to sensible state after crapping out in raw mode.
 */
#include <sgtty.h>

main()
{
	struct sgttyb buf;
	struct tchars tbuf;
	struct ltchars ltbuf;

	ioctl(2, TIOCGETP, &buf);
	ioctl(2, TIOCGETC, &tbuf);
	ioctl(2, TIOCGLTC, &ltbuf);
	buf.sg_flags &= ~(RAW|CBREAK|VTDELAY|ALLDELAY);
	buf.sg_flags |= XTABS|ECHO|CRMOD|ANYP;
	reset(&buf.sg_erase, CERASE);
	reset(&buf.sg_kill, CKILL);
	reset(&tbuf.t_intrc, CINTR);
	reset(&tbuf.t_quitc, CQUIT);
	reset(&tbuf.t_startc, CSTART);
	reset(&tbuf.t_stopc, CSTOP);
	reset(&tbuf.t_eofc, CEOF);
	reset(&ltbuf.t_suspc, CSUSP);
	reset(&ltbuf.t_dsuspc, CDSUSP);
	reset(&ltbuf.t_rprntc, CRPRNT);
	reset(&ltbuf.t_flushc, CFLUSH);
	reset(&ltbuf.t_lnextc, CLNEXT);
	reset(&ltbuf.t_werasc, CWERASE);
	/* brkc is left alone */
	ioctl(2, TIOCSETN, &buf);
	ioctl(2, TIOCSETC, &tbuf);
	ioctl(2, TIOCSLTC, &ltbuf);
	execlp("tset", "tset", "-Q", "-I", 0);	/* fix term dependent stuff */
	exit(1);
}

reset(cp, def)
	char *cp;
	int def;
{

	if (*cp == 0 || (*cp&0377)==0377)
		*cp = def;
}

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
static	char	rcsid[] = "$Header: consio.c 2.6 90/11/08 $";
#endif

/*
 * consio.c
 *	Machine dependent cnputc(), putflush() implementation.
 *
 * Also, getchar for debug.
 */

/* $Log:	consio.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"

#include "../balance/engine.h"
#include "../balance/cfg.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"

static	char	pclastc[MAXNUMCPU];/* last char output, for putprocid */

int	putprocid = 1;		/* default: pre-pend proc-ID to output lines */

extern	short	upyet;		/* says if init done yet */
extern  u_char  cons_slic;      /* SLIC id of console device */

lock_t	prf_lock;		/* lock for mutual exclusion in printf */

#ifdef	DEBUG
/*
 * Data for DEBUG console....
 */
sema_t		dc_mutex;		/* semaphore for dumb console */
					/* and dinfo.c */
char		gc_last;		/* last input character */
#endif	DEBUG

/*
 * initmonio()
 *	Init monitor IO.
 */

initmonio()
{
	register int i;

	for(i = 0; i < MAXNUMCPU; i++) {
		pclastc[i] = '\n';
	}
	init_lock(&prf_lock, G_MONIO);
#ifdef	DEBUG
	init_sema(&dc_mutex, 1, 0, G_MONIO);
#endif	DEBUG
}

/*
 * cnputc()
 *	Arrange that a character be output on the "console".
 *
 */

/*ARGSUSED*/
cnputc(c)
	char	c;
{
	register int me;
	spl_t	s;

	if (c == '\n')
		cnputc('\r');

	if (!upyet) {
		sq_putc(c);
		return;
	}

	me = l.me;

	/*
	 * If desired, pre-pend processor # to output lines.
	 */

	if (putprocid && pclastc[me] == '\n') {
		pclastc[me] = '\0';
		cnputc('0' + (me/10));
		cnputc('0' + (me%10));
		cnputc(':');
		cnputc(' ');
	}

	/*
	 * Output character.
	 */

	s = splhi();				/* mutex SLIC usage */

	sq_putc(c);
	pclastc[me] = c;

	splx(s);
}

/*
 * sq_putc()
 *	Local version to do SCSI output of character.
 *
 * Assumes caller did splhi() or otherwise blocks interrupts.
 * Can't do this here, since this used at boot time before
 * turned on kernel page-tables (==> splhi(), splx() use wrong
 * virt-addr for SLIC).
 */

sq_putc(c)
	char	c;
{
#ifdef	i386
#ifdef	DEBUG
	if (upyet) l.holdgate = 0;			/* avoid panic loop */
#endif	DEBUG
#endif	i386
        mIntr(cons_slic, PUTCHAR_BIN, (u_char)c);
}

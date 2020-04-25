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
static	char	rcsid[] = "$Header: monop.c 2.6 87/04/27 $";
#endif

/*
 * Monoprocessor driver support.
 *	- sleep/wakeup emulation using semaphores.
 *	- [b,c]mpdevsw routines.
 */

/* $Log:	monop.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"

#include "../balance/slic.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#ifdef	i386
#include "../machine/hwparam.h"
#include "../machine/vmparam.h"
#endif	i386

#define	SQSIZE	32	/* Must be power of 2 */
#define	SLPHASH(x) (&slpsemas[((int)x >> 5) & (SQSIZE-1)])

sema_t slpsemas[SQSIZE];

extern int bmpmap[], cmpmap[];		/* mapping [b,c]devsw to [b,c]mpdevsw */
extern struct bdevsw bmpdevsw[];	/* bdevsw mono processor switch table */
extern struct cdevsw cmpdevsw[];	/* cdevsw mono processor switch table */

extern int mono_P_eng;		/* engine where monop drivers run */

/*
 * Initialize sleep/wakeup semaphores.
 */
slpboot()
{
	register sema_t *semptr;

	for (semptr = slpsemas; semptr < &slpsemas[SQSIZE]; semptr++)
		init_sema(semptr, 0, 0, G_SLPWAKE);
}

/*
 * setmonop()
 *	Setup mono_P_slic so that device driver timeout events are bound to
 *	run on the same processor.
 *
 * This is for mono-processor pseudo-device drivers to call in their xxboot()
 * routines.
 *
 * NOTE: Multibus (mbad) and SCSI (sec) mono-processor device drivers need
 * not call this routine as the probe_xxx_devices routines will set the
 * mono_P_slic variable.
 */
setmonop()
{
	extern int mono_P_slic;

	mono_P_slic = va_slic->sl_procid;
}

/*
 * Give up the processor till a wakeup occurs
 * on chan, at which time the process
 * enters the scheduling queue at priority pri.
 * The most important effect of pri is that when
 * pri<=PZERO a signal cannot disturb the sleep;
 * if pri>PZERO signals will be processed.
 * Callers of this routine must be prepared for
 * premature return, and check that the reason for
 * sleeping has gone away.
 */
sleep(chan, pri)
	caddr_t chan;
	int pri;
{
	u.u_procp->p_yawc = chan;
	p_sema(SLPHASH(chan), pri);
}

/*
 * Wake up all processes sleeping on chan.
 */
wakeup(chan)
	register caddr_t chan;
{
	register struct proc *p;
	register sema_t *semptr;
	register spl_t s_ipl;

	semptr = SLPHASH(chan);
	lockup_sema(semptr, s_ipl);
	p = semptr->s_head;

	while ((sema_t *)p != semptr) {
		if (p->p_yawc == chan) {
			/*
			 * Signals can happen between unlock_sema
			 * and p_lock of process state.
			 */
			unlock_sema(semptr, SPLHI);
#ifdef	i386
			/*
			 * NOTE: SGS lockup_sema(), unlock_sema() don't
			 * fuss with "s_ipl", since just disable processor
			 * interrupts locally.  Thus, unlock_sema() above
			 * drops SPL to what it was at entry, and p_lock()
			 * must re-set s_ipl.  Ok to allow interrupt in
			 * between these, caller arranges no relevant
			 * interrupts via spl's.
			 */
			s_ipl = p_lock(&p->p_state, SPLHI);
#endif	i386
#ifdef	ns32000
			(void) p_lock(&p->p_state, SPLHI);
#endif	ns32000
			/*
			 * recheck in case signal won the race.
			 */
			if (p->p_wchan == semptr) {
				force_v_sema(p);
				p->p_flag &= ~SIGWOKE;
			}
			v_lock(&p->p_state, s_ipl);
			lockup_sema(semptr, s_ipl);
			p = semptr->s_head;
		} else
			p = p->p_rlink;
	}
	unlock_sema(semptr, s_ipl);
}

/*
 * Macros to setup affinity and restore affinity.
 */

#ifdef lint
#define SUL oldeng = ANYENG;
#else
#define SUL
#endif lint

#define SETUP_AFF		\
	int	oldeng, retval;	\
	label_t	lqsave;		\
				\
	SUL			\
	/*			\
	 * Catch signals so that affinity can be undone.	\
	 */			\
	lqsave = u.u_qsave;	\
	if (setjmp(&u.u_qsave)) {	\
		u.u_qsave = lqsave;	\
		(void)runme(oldeng);	\
		longjmp(&u.u_qsave);	\
	}			\
	/*			\
	 * switch to monoprocessor engine;	\
	 * call devsw routine; and return to previous affinity.	\
	 */			\
	oldeng = runme(mono_P_eng)

#define RESTORE_AFF		\
	(void)runme(oldeng);	\
	u.u_qsave = lqsave;	\
	return (retval)

/*
 * Mono processor intermediate code for bdevsw entries.
 * These include: open, close, strategy and minphys.
 */

bmpopen(dev, mode)
	dev_t	dev;
	int	mode;
{
	SETUP_AFF;
	retval = (*bmpdevsw[bmpmap[major(dev)]].d_open)(dev, mode);
	RESTORE_AFF;
}

bmpclose(dev, flag)
	dev_t	dev;
	int	flag;
{
	SETUP_AFF;
	retval = (*bmpdevsw[bmpmap[major(dev)]].d_close)(dev, flag);
	RESTORE_AFF;
}

bmpstrat(bp)
	struct	buf	*bp;
{
	SETUP_AFF;
	retval = (*bmpdevsw[bmpmap[major(bp->b_dev)]].d_strategy)(bp);
	RESTORE_AFF;
}

bmpminphys(bp)
	struct	buf	*bp;
{
	SETUP_AFF;
	retval = (*bmpdevsw[bmpmap[major(bp->b_dev)]].d_minphys)(bp);
	RESTORE_AFF;
}

/*
 * Mono processor intermediate code for cdevsw entries.
 * These include: open, close, read, write, select, and ioctl.
 */

cmpopen(dev, mode)
	dev_t	dev;
	int	mode;
{
	SETUP_AFF;
	retval = (*cmpdevsw[cmpmap[major(dev)]].d_open)(dev, mode);
	RESTORE_AFF;
}

cmpclose(dev, flag)
	dev_t	dev;
	int	flag;
{
	SETUP_AFF;
	retval = (*cmpdevsw[cmpmap[major(dev)]].d_close)(dev, flag);
	RESTORE_AFF;
}

cmpread(dev, uio)
	dev_t	dev;
	struct	uio *uio;
{
	SETUP_AFF;
	retval = (*cmpdevsw[cmpmap[major(dev)]].d_read)(dev, uio);
	RESTORE_AFF;
}

cmpwrite(dev, uio)
	dev_t	dev;
	struct	uio *uio;
{
	SETUP_AFF;
	retval = (*cmpdevsw[cmpmap[major(dev)]].d_write)(dev, uio);
	RESTORE_AFF;
}

cmpselect(dev, flag)
	dev_t	dev;
	int	flag;
{
	SETUP_AFF;
	retval = (*cmpdevsw[cmpmap[major(dev)]].d_select)(dev, flag);
	RESTORE_AFF;
}

cmpioctl(dev, cmd, data, flag)
	dev_t	dev;
	int	cmd;
	caddr_t	data;
	int	flag;
{
	SETUP_AFF;
	retval = (*cmpdevsw[cmpmap[major(dev)]].d_ioctl)(dev, cmd, data, flag);
	RESTORE_AFF;
}

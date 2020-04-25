/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
static	char	rcsid[] = "$Header: tty.c 2.29 1991/09/25 22:29:16 $";
#endif

/* $Log: tty.c,v $
 *
 */

#include "../machine/reg.h"

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/tty.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/vmmeter.h"	/* for plocal.h */
#include "../h/vmsystm.h"	/* for plocal.h */
#include "../h/cmn_err.h"

#include "../machine/pte.h"	/* for plocal.h */
#include "../machine/vmparam.h"	/* for plocal.h */
#include "../machine/plocal.h"
#include "../machine/intctl.h"

#define	mstohz(ms)	(((ms) * hz) >> 10)

/* Sentinal character for literal-nexting and backslash-quoting */

#define LITCHAR		0x8e

/*
 * Table giving parity for characters and indicating
 * character classes to tty driver.  In particular,
 * if the low 6 bits are 0, then the character needs
 * no special processing on output.
 */

char partab[] = {
	0001,0201,0201,0001,0201,0001,0001,0201,
	0202,0004,0003,0201,0005,0206,0201,0001,
	0201,0001,0001,0201,0001,0201,0201,0001,
	0001,0201,0201,0001,0201,0001,0001,0201,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0200,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0200,0000,0000,0200,0000,0200,0200,0000,
	0000,0200,0200,0000,0200,0000,0000,0201,

	/*
	 * 7 bit ascii ends with the last character above,
	 * but we continue through all 256 codes for the sake
	 * of the tty output routines which use special vax
	 * instructions which need a 256 character trt table.
	 * Left in for NS32000 Series.
	 */

	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007,
	0007,0007,0007,0007,0007,0007,0007,0007
};

/*
 * Input mapping table-- if an entry is non-zero, when the
 * corresponding character is typed preceded by "\" the escape
 * sequence is replaced by the table value.  Mostly used for
 * upper-case only terminals.
 */
char	maptab[] ={
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,'|',000,000,000,000,000,'`',
	'{','}',000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,000,000,
	000,000,000,000,000,000,'~',000,
	000,'A','B','C','D','E','F','G',
	'H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W',
	'X','Y','Z',000,000,000,000,000,
};

short	tthiwat[NSPEEDS] =
   { 100,100,100,100,100,100,100,200,200,400,400,400,650,650,1300,2000 };
short	ttlowat[NSPEEDS] =
   {  30, 30, 30, 30, 30, 30, 30, 50, 50,120,120,120,125,125,125,125 };

struct	ttychars ttydefaults = {
	CERASE,	CKILL,	CINTR,	CQUIT,	CSTART,	CSTOP,	CEOF,
	CBRK,	CSUSP,	CDSUSP, CRPRNT, CFLUSH, CWERASE,CLNEXT
};

/*
 * ttyinit() - called from driver boot routine.
 *	function for drivers to init mutex fields of the tty structure.
 *	currently assumes same gate used for t_ttylock, t_rawqwait, and
 *	t_outqwait.
 */

/*ARGSUSED*/
ttyinit(tp, gate)
	struct tty	*tp;
	gate_t		gate;
{
	init_lock(&tp->t_ttylock, gate);		/* lock tty structure */
	init_sema(&tp->t_rawqwait, 0, 0, gate);		/* rawq wait */
	init_sema(&tp->t_outqwait, 0, 0, gate);		/* outq wait */
}

/*
 * ttychars()
 * called from driver open routine on 1st open to initialize t_chars.
 *
 * t_ttylock is assumed locked at SPLTTY by the caller.
 */
ttychars(tp)
	struct tty *tp;
{

	tp->t_chars = ttydefaults;
	tp->t_rlitcount = 0;
	tp->t_clitcount = 0;
}

/*
 * Wait for output to drain, then flush input waiting.
 *
 * t_ttylock is assumed locked at SPLTTY by the caller.
 */
ttywflush(tp)
	register struct tty *tp;
{

	ttywait(tp);
	ttyflush(tp, FREAD);
}


/*
 * Check the output queue on tp for space for a kernel message
 * (from uprintf/tprintf).  Allow some space over the normal
 * hiwater mark so we don't lose messages due to normal flow
 * control, but don't let the tty run amok.
 * Sleeps here are not interruptible, but we return prematurely
 * if new signals come in.
 */

int uprintf_hiwat = 2 * OBUFSIZ;	/* make tunable for now */

ttycheckoutq(tp, wait)
	register struct tty *tp;
	int wait;
{
	int hiwat, oldsig;
	spl_t	s;

	hiwat = TTHIWAT(tp);
	s = p_lock(&tp->t_ttylock, SPLTTY);
	oldsig = u.u_procp->p_sig;
	if (tp->t_outq.c_cc > hiwat + uprintf_hiwat) {
		while (tp->t_outq.c_cc > hiwat) {
			ttstart(tp);
			if (wait == 0 || u.u_procp->p_sig != oldsig) {
				v_lock(&tp->t_ttylock, s);
				return (0);
			}
			tp->t_state |= TS_ASLEEP;
			/*
			 * Drop lock before call to timeout.
			 */
			v_lock(&tp->t_ttylock, s);
#ifndef lint /* lint dosn't seem to understand this cast */
			timeout((int(*)())v_sema, (caddr_t)&tp->t_outqwait, hz);
#endif
			/*
			 * wait for output or 1 second.
			 */
			p_sema(&tp->t_outqwait, TTOPRI);
			(void) p_lock(&tp->t_ttylock, SPLTTY);
		}
	}
	v_lock(&tp->t_ttylock, s);
	return (1);
}

/*
 * Wait for output to drain.
 *
 * t_ttylock is assumed locked at SPLTTY by the caller.
 */
ttywait(tp)
	register struct tty *tp;
{
	while ((tp->t_outq.c_cc || tp->t_state&TS_BUSY) &&
	       tp->t_state&TS_CARR_ON && tp->t_oproc) {
		(*tp->t_oproc)(tp);
		tp->t_state |= TS_ASLEEP;
		p_sema_v_lock(&tp->t_outqwait, TTOPRI, &tp->t_ttylock, SPL0);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
	}
}

/*
 * Flush all TTY queues
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 * Note that if performance is an issue one should use
 * ndflush to flush queues.
 */
ttyflush(tp, rw)
	register struct tty *tp;
{

	if (rw & FWRITE) {
		/*
		 * clear TS_TTSTOP so driver will mark TS_FLUSH
		 * if output in progress.
		 */
		tp->t_state &= ~TS_TTSTOP;
		(*cdevsw[major(tp->t_dev)].d_stop)(tp, rw);
		while (getc(&tp->t_outq) >= 0)
			continue;
		/*
		 * Awaken those wanting to write.
		 */
		vall_sema(&tp->t_outqwait);
	}
	if (rw & FREAD) {
		/* flush canonical queue */
		while (getc(&tp->t_canq) >= 0)
			continue;
		/* flush raw queue */
		while (getc(&tp->t_rawq) >= 0)
			continue;
		tp->t_rocount = 0;
		tp->t_rocol = 0;
		tp->t_clitcount = 0;
		tp->t_rlitcount = 0;
		tp->t_state &= ~TS_LOCAL;
		vall_sema(&tp->t_rawqwait);
	}
}

/*
 * Send stop character on input overflow.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttyblock(tp)
	register struct tty *tp;
{
	register x;

	x = tp->t_rawq.c_cc + tp->t_canq.c_cc;
	if (tp->t_rawq.c_cc > TTYHOG) {
		ttyflush(tp, FREAD|FWRITE);
		tp->t_state &= ~TS_TBLOCK;
	}
	/*
	 * Block further input iff:
	 * Current input > threshold AND input is available to user program
	 */
	if (x >= TTYHOG/2
	    && ((tp->t_flags&(RAW|CBREAK)) || tp->t_canq.c_cc > 0)) {
		if (putc(tp->t_stopc, &tp->t_outq)==0) {
			tp->t_state |= TS_TBLOCK;
			ttstart(tp);
		}
	}
}

/*
 * Restart typewriter output following a delay
 * timeout.
 * The name of the routine is passed to the timeout
 * subroutine and it is called during a clock interrupt.
 */
ttrstrt(tp)
	register struct tty *tp;
{
	spl_t s_ipl;

	if (tp == 0) {
		panic("ttrstrt");
		/*
		 *+ A timeout on ttrstrt was called with null argument.
		 */
	}
	s_ipl = p_lock(&tp->t_ttylock, SPLTTY);
	tp->t_state &= ~TS_TIMEOUT;
	ttstart(tp);
	v_lock(&tp->t_ttylock, s_ipl);
}

/*
 * Start output on the typewriter. It is used from the top half
 * after some characters have been put on the output queue,
 * from the interrupt routine to transmit the next
 * character, and after a timeout has finished.
 *
 * Called with ttylock locked at SPLTTY.
 */
ttstart(tp)
	register struct tty *tp;
{

	if (tp->t_oproc)		/* kludge for pty */
		(*tp->t_oproc)(tp);
}

/*
 * Common code for tty ioctls.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
/*ARGSUSED*/
ttioctl(tp, com, data, flag)
	register struct tty *tp;
	caddr_t data;
{
	extern int nldisp;
	register int newflags;
	short pgrp;		/* snapshot pgrp because of race with setpgrp */

	/*
	 * If the ioctl involves modification,
	 * hang if in the background.
	 */
	switch (com) {

	case TIOCSETD:
	case TIOCSETP:
	case OLD_TIOCSETP:
	case TIOCSETN:
	case OLD_TIOCSETN:
	case TIOCFLUSH:
	case OLD_TIOCFLUSH:
	case TIOCSETC:
	case TIOCSLTC:
	case TIOCSPGRP:
	case TIOCLBIS:
	case TIOCLBIC:
	case TIOCLSET:
	case TIOCSTI:
	case TIOCSWINSZ:
		while(tp->t_line == NTTYDISC &&
		      (((pgrp = u.u_procp->p_pgrp) != tp->t_pgrp) && pgrp) &&
		      tp == u.u_ttyp && (u.u_procp->p_flag&SVFORK) == 0 &&
		      !(u.u_procp->p_sigignore & sigmask(SIGTTOU)) &&
		      !(u.u_procp->p_sigmask & sigmask(SIGTTOU))) {
			gsignal(pgrp, SIGTTOU);
			/*
			 * take the stop signal, longjmp if other signal
			 * present.
			 */
			v_lock(&tp->t_ttylock, SPL0);
			(void) p_lock(&u.u_procp->p_state, SPLHI);
			if (issig((lock_t *)NULL) > 0) {
				v_lock(&u.u_procp->p_state, SPL0);
				longjmp(&u.u_qsave);
			}
			v_lock(&u.u_procp->p_state, SPL0);
			(void) p_lock(&tp->t_ttylock, SPLTTY);
			/* now check again... */
		}
		break;
	}

	/*
	 * Process the ioctl.
	 */
	switch (com) {

	/* get discipline number */
	case TIOCGETD:
		*(int *)data = tp->t_line;
		break;

	/* set line discipline */
	case TIOCSETD: {
		register int t = *(int *)data;
		int error = 0;
		char oldline;

		if ((unsigned)t >= nldisp)
			return (ENXIO);
		if (t != tp->t_line) {
			oldline = tp->t_line;
			(*linesw[tp->t_line].l_close)(tp);
			error = (*linesw[t].l_open)(tp->t_dev, tp);
			if (error) {
				(void) (*linesw[oldline].l_open)(tp->t_dev, tp);
				tp->t_line = oldline;
				return (error);
			}
			tp->t_line = t;
		}
		break;
	}

	/* prevent more opens on channel */
	case TIOCEXCL:
		tp->t_state |= TS_XCLUDE;
		break;

	case TIOCNXCL:
		tp->t_state &= ~TS_XCLUDE;
		break;

	/* hang up line on last close */
	case TIOCHPCL:
		tp->t_state |= TS_HUPCLS;
		break;

	case OLD_TIOCFLUSH: 
	case TIOCFLUSH: {
		register int flags = *(int *)data;

		if (flags == 0)
			flags = FREAD|FWRITE;
		else
			flags &= FREAD|FWRITE;
		ttyflush(tp, flags);
		break;
	}

	/* return number of characters immediately available */
	case FIONREAD:
		*(off_t *)data = ttnread(tp);
		break;

	case TIOCOUTQ:
		*(int *)data = tp->t_outq.c_cc;
		break;

	case TIOCSTOP:
		if ((tp->t_state&TS_TTSTOP) == 0) {
			tp->t_state |= TS_TTSTOP;
			(*cdevsw[major(tp->t_dev)].d_stop)(tp, 0);
		}
		break;

	case TIOCSTART:
		if ((tp->t_state&TS_TTSTOP) || (tp->t_flags&FLUSHO)) {
			tp->t_state &= ~TS_TTSTOP;
			tp->t_flags &= ~FLUSHO;
			ttstart(tp);
		}
		break;

	/*
	 * Simulate typing of a character at the terminal.
	 */
	case TIOCSTI:
		if (u.u_uid && (flag & FREAD) == 0)
			return (EPERM);
		if (u.u_uid && u.u_ttyp != tp)
			return (EACCES);
		(*linesw[tp->t_line].l_rint)(*(char *)data, tp);
		break;

	case OLD_TIOCSETP:
	case TIOCSETP:
	case OLD_TIOCSETN: 
	case TIOCSETN: {
		register struct sgttyb *sg = (struct sgttyb *)data;

		tp->t_erase = sg->sg_erase;
		tp->t_kill = sg->sg_kill;
		tp->t_ispeed = sg->sg_ispeed;
		tp->t_ospeed = sg->sg_ospeed;
		if ((com == TIOCSETN) || (com == TIOCSETP))
			newflags = (tp->t_flags&0xffff0000) | (sg->sg_flags);
		else
			newflags = (tp->t_flags&0xffff0000) | (sg->sg_flags&0xffff);
		if (tp->t_flags&RAW || newflags&RAW || com == TIOCSETP ||
						com == OLD_TIOCSETP ) {
			ttywait(tp);
			ttyflush(tp, FREAD);
		} else if ((tp->t_flags&CBREAK) != (newflags&CBREAK)) {
			if (newflags&CBREAK) {
				struct clist tq;

				catq(&tp->t_rawq, &tp->t_canq);
				tp->t_clitcount += tp->t_rlitcount;
				tp->t_rlitcount = 0;
				tq = tp->t_rawq;
				tp->t_rawq = tp->t_canq;
				tp->t_canq = tq;
			} else {
				tp->t_flags |= PENDIN;
				newflags |= PENDIN;
				ttwakeup(tp);
			}
		}
		tp->t_flags = newflags;
		if (tp->t_flags&RAW) {
			tp->t_state &= ~TS_TTSTOP;
			ttstart(tp);
		}
		break;
	}

	/* send current parameters to user */
	case OLD_TIOCGETP:
	case TIOCGETP: {
		register struct sgttyb *sg = (struct sgttyb *)data;

		sg->sg_ispeed = tp->t_ispeed;
		sg->sg_ospeed = tp->t_ospeed;
		sg->sg_erase = tp->t_erase;
		sg->sg_kill = tp->t_kill;
		sg->sg_flags = tp->t_flags & 0xffff;
		break;
	}

	case FIONBIO:
		if (*(int *)data)
			tp->t_state |= TS_NBIO;
		else
			tp->t_state &= ~TS_NBIO;
		break;

	case FIOASYNC:
		if (*(int *)data)
			tp->t_state |= TS_ASYNC;
		else
			tp->t_state &= ~TS_ASYNC;
		break;

	case TIOCGETC:
		bcopy((caddr_t)&tp->t_intrc, data, sizeof(struct tchars));
		break;

	case TIOCSETC:
		bcopy(data, (caddr_t)&tp->t_intrc, sizeof(struct tchars));
		break;

	/* set/get local special characters */
	case TIOCSLTC:
		/*
		 * silently disallow t_lnextc to be set to LITCHAR, else
		 * literal-nexting will break.
		 */
#undef	t_lnextc
		if (((struct ltchars *) data)->t_lnextc == LITCHAR)
			((struct ltchars *)data)->t_lnextc =
				tp->t_chars.tc_lnextc;
#define t_lnextc        t_chars.tc_lnextc

		bcopy(data, (caddr_t)&tp->t_suspc, sizeof(struct ltchars));
		break;

	case TIOCGLTC:
		bcopy((caddr_t)&tp->t_suspc, data, sizeof(struct ltchars));
		break;

	/*
	 * Modify local mode word.
	 */
	case TIOCLBIS:
		tp->t_flags |= *(int *)data << 16;
		break;

	case TIOCLBIC:
		tp->t_flags &= ~(*(int *)data << 16);
		break;

	case TIOCLSET:
		tp->t_flags &= 0xffff;
		tp->t_flags |= *(int *)data << 16;
		break;

	case TIOCLGET:
		*(int *)data = ((unsigned)tp->t_flags) >> 16;
		break;

	/* should allow SPGRP and GPGRP only if tty open for reading */
	case TIOCSPGRP:
		tp->t_pgrp = *(int *)data;
		break;

	case TIOCGPGRP:
		*(int *)data = tp->t_pgrp;
		break;

	case TIOCSWINSZ:
		if (bcmp((caddr_t)&tp->t_winsize, data, sizeof(struct winsize))) {
			tp->t_winsize = *(struct winsize *)data;
			gsignal(tp->t_pgrp, SIGWINCH);
		}
		break;

	case TIOCGWINSZ:
		*(struct winsize *)data = tp->t_winsize;
		break;

#ifdef	TTYMON
	case TIOCSMON:			/* start tty monitoring to file */
		if (u.u_uid == 0) {
			struct vnode *vp;
			int error;

			v_lock(&tp->t_ttylock, SPL0);
			error = lookupname(*(caddr_t *)data, UIOSEG_USER, 1,
				(struct vnode **)0, &vp);
			(void) p_lock(&tp->t_ttylock, SPLTTY);
			if (error)
				return(error);
			if (tp->t_vp) {
				error = EBUSY;
			} else if (vp->v_type != VREG) {
				error = EINVAL;
			}
			if (error) {
				VN_PUT(vp);
				return (error);
			} else {
				tp->t_vp = vp;
				VN_UNLOCKNODE(vp);
			}
		} else
			return(EPERM);
		break;

	case TIOCNMON:				/* Turn off tty monitoring */
		if (u.u_uid == 0)
			return(ttymon_off(tp));
		else
			return(EPERM);
#endif	TTYMON

	default:
		return (-1);
	}
	return (0);
}

#ifdef	TTYMON
/*
 * Turn off tty monitoring. 
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */

ttymon_off(tp)
	register struct tty *tp;
{
	register struct vnode *vp;

	if ((vp = tp->t_vp) == NULL)
		return(0);
	tp->t_vp = NULL;
	v_lock(&tp->t_ttylock, SPL0);
	VN_RELE(vp);
	(void) p_lock(&tp->t_ttylock, SPLTTY);
	return(0);
}

/*
 * Do ttymon I/O
 *
 * Assumes called with t_ttylock locked at SPLTTY.
 * Assumes tp->t_vp is valid.
 */

ttymon(tp, addr, cnt)
	struct tty *tp;
	caddr_t addr;
	int cnt;
{
	register struct vnode *vp;
	int flimit, error;

	vp = tp->t_vp;

	/*
	 * Try to lock vnode; if can't, sleep for awhile and try again.
	 */
	while (!VN_TRYLOCKNODE(vp)) {		/* get vnode */
		v_lock(&tp->t_ttylock, SPL0);
		p_sema(&lbolt, TTOPRI);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
		if ((vp = tp->t_vp) == NULL)
			return;				/* monitor is off */
	}
		
	v_lock(&tp->t_ttylock, SPL0);

	flimit = u.u_rlimit[RLIMIT_FSIZE].rlim_cur;
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = RLIM_INFINITY;
	error = vn_rdwr(UIO_WRITE, vp, addr, cnt, 0, UIOSEG_KERNEL,
							IO_APPEND, (int *)0);
	u.u_rlimit[RLIMIT_FSIZE].rlim_cur = flimit;

	VN_UNLOCKNODE(vp);
	(void) p_lock(&tp->t_ttylock, SPLTTY);
	/* If error or zero link count, quit monitoring  */
	if (error || (vp->v_flag & VNOLINKS)) {
		(void) ttymon_off(tp);
	}
}
#endif	TTYMON

/*
 * Do nothing specific version of line
 * discipline specific ioctl command.
 */
/*ARGSUSED*/
nullioctl(tp, cmd, data, flags)
	struct tty *tp;
	char *data;
	int flags;
{

#ifdef lint
	tp = tp; data = data; flags = flags;
#endif
	return (-1);
}

/*
 * Return the number of characters ready to be read.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttnread(tp)
	struct tty *tp;
{
	int nread = 0;

	if (tp->t_flags & PENDIN)
		ttypend(tp);
	nread = tp->t_canq.c_cc;
	/*
	 * In raw mode add rawq.
	 * In cbreak mode we would have added litchars to esacpe litchars
	 * so exclude them from the raw count.
	 * In canonical mode literal escape characters need to be excluded from
	 * the count if they are already in canq.
	 */
	if (tp->t_flags & RAW) {
		nread += tp->t_rawq.c_cc;
	} else if (tp->t_flags & CBREAK) {
		nread += tp->t_rawq.c_cc;
		nread -= tp->t_rlitcount;
		nread -= tp->t_clitcount;
	} else 
		nread -= tp->t_clitcount;
	return (nread);
}

/*
 * Return 1 if I/O pending, otherwise return 0.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttselect(tp, rw)
	register struct tty *tp;
	int rw;
{
	spl_t	s_ipl;

	switch (rw) {

	case FREAD:
		if (ttnread(tp) > 0 || ((tp->t_state & TS_CARR_ON) == 0))
			return (1);

		if (tp->t_rsel) {
			/*
			 * If the current process is already known to be
			 * selecting, then nothing more to do. This can
			 * happen if t_rsel contains stale data from a
			 * previous call to select from this process.
			 * This avoids false collisions.
			 */
			if (tp->t_rsel == u.u_procp)
				break;

			/*
			 * If 1st selector is waiting on selwait or
			 * has not yet blocked on selwait, then we
			 * have a collision. It is sufficient to lock
			 * only the select_lck since selwakeup cannot
			 * happen without also locking select_lck.
			 */
			s_ipl = p_lock(&select_lck, SPL6);
			if (tp->t_rsel->p_wchan == &selwait ||
			   (tp->t_rsel->p_flag & SSEL))
				tp->t_state |= TS_RCOLL;
			else
				tp->t_rsel = u.u_procp;
			v_lock(&select_lck, s_ipl);
		} else
			tp->t_rsel = u.u_procp;
		break;

	case FWRITE:
		if (tp->t_outq.c_cc <= TTLOWAT(tp))
			return (1);

		if (tp->t_wsel) {
			if (tp->t_wsel == u.u_procp)
				break;

			s_ipl = p_lock(&select_lck, SPL6);
			if (tp->t_wsel->p_wchan == &selwait ||
			   (tp->t_wsel->p_flag & SSEL))
				tp->t_state |= TS_WCOLL;
			else
				tp->t_wsel = u.u_procp;
			v_lock(&select_lck, s_ipl);
		} else
			tp->t_wsel = u.u_procp;
		break;
	}
	return (0);
}

/*
 * Initial open of tty, or (re)entry to line discipline.
 * Establish a process group for distribution of
 * quits and interrupts from the tty.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttyopen(dev, tp)
	dev_t dev;
	register struct tty *tp;
{
	register struct proc *pp;

	pp = u.u_procp;
	tp->t_dev = dev;
	(void) p_lock(&pp->p_state, SPLHI);
	if (pp->p_pgrp == 0) {
		u.u_ttyp = tp;
		u.u_ttyd = dev;
		if (tp->t_pgrp == 0)
			tp->t_pgrp = pp->p_pid;
		pp->p_pgrp = tp->t_pgrp;
	}
	v_lock(&pp->p_state, SPLTTY);
	if ((tp->t_state & TS_ISOPEN) == 0) {
		/*
		 * First open.
		 */
		bzero((caddr_t)&tp->t_winsize, sizeof(struct winsize));
		if (tp->t_line != NTTYDISC)
			ttywflush(tp);
		tp->t_state |= TS_ISOPEN;
	}
	tp->t_state &= ~TS_WOPEN;	/* ttywflush may block on t_outqwait */
	return (0);
}

/*
 * "close" a line discipline
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttylclose(tp)
	register struct tty *tp;
{
	label_t lqsave;

	lqsave = u.u_qsave;
	if (setjmp(&u.u_qsave)) {			/* catch half closes */
		/*
		 * Signalled out of ttywait.
		 * Flush clists and clean up.
		 * Will not happen if last close is during exit since
		 * signals have already been disabled.
		 */
		u.u_error = EINTR;
		(void) p_lock(&tp->t_ttylock, SPLTTY);
		ttyflush(tp, FREAD|FWRITE);		/* flush clists */
		tp->t_state &= ~TS_BUSY;		/* output done! */
		tp->t_line = 0;
		u.u_qsave = lqsave;
		return;					/* return to driver */
	}

	ttywflush(tp);
	u.u_qsave = lqsave;
	tp->t_line = 0;
}

/*
 * Clean tp on LAST close
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttyclose(tp)
	register struct tty *tp;
{
	tp->t_pgrp = 0;
	ttyflush(tp, FREAD|FWRITE);		/* flush clists */
	tp->t_rsel = (struct proc *)NULL;	/* Clear select data */
	tp->t_wsel = (struct proc *)NULL;	/* Clear select data */
	tp->t_state = 0;			/* Clears TS_LCLOSE */

	/* Allow pending opens to occur */
	if (tp->t_nopen)
		vall_sema(&tp->t_rawqwait);
}

/*
 * reinput pending characters after state switch
 * call at spltty().
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttypend(tp)
	register struct tty *tp;
{
	struct clist tq;
	register c;

	tp->t_flags &= ~PENDIN;
	tp->t_state |= TS_TYPEN;
	tq = tp->t_rawq;
	tp->t_rawq.c_cc = 0;
	tp->t_rawq.c_cf = tp->t_rawq.c_cl = 0;
	while ((c = getc(&tq)) >= 0)
		ttyinput(c, tp);
	tp->t_state &= ~TS_TYPEN;
}

/*
 * Place a character on raw TTY input queue,
 * putting in delimiters and waking up top
 * half as needed.  Also echo if required.
 * The arguments are the character and the
 * appropriate tty structure.
 *
 * Called from device reader interrupt handler. Assumes
 * t_ttylock is locked at SPLTTY.
 */
ttyinput(c, tp)
	register c;
	register struct tty *tp;
{
	register int t_flags;
	int i, ret;

	t_flags = tp->t_flags;
	/*
	 * If input is pending take it first.
	 */
	if (t_flags&PENDIN)
		ttypend(tp);
	l.cnt.v_ttyin++;
	c &= 0377;

	/*
	 * In tandem mode, check high water mark.
	 */
	if (t_flags&TANDEM)
		ttyblock(tp);

	if (t_flags&RAW) {
		/*
		 * Raw mode, just put character
		 * in input q w/o interpretation.
		 */
		if (tp->t_rawq.c_cc > TTYHOG) 
			ttyflush(tp, FREAD|FWRITE);
		else {
			if (putc(c, &tp->t_rawq) >= 0)
				ttwakeup(tp);
			ttyecho(c, tp);
		}
		goto endcase;
	}

	/*
	 * Ignore any high bit added during
	 * previous ttyinput processing.  This has the
	 * effect of stripping off parity bits.
	 */
	if (((tp->t_state&TS_TYPEN) == 0) && ((t_flags&PASS8) == 0))
		c &= 0177;
	/*
	 * Check for literal nexting very first
	 */
	if (tp->t_state&TS_LNCH) {
		tp->t_state |= TS_LITCHR;
		tp->t_state &= ~TS_LNCH;
	}

	/*
	 * Scan for special characters.  This code
	 * is really just a big case statement with
	 * non-constant cases.  The bottom of the
	 * case statement is labeled ``endcase'', so goto
	 * it after a case match, or similar.
	 */
	if (tp->t_line == NTTYDISC && !(tp->t_state&TS_LITCHR)) {
		if (c == tp->t_lnextc) {
			if (t_flags&ECHO)
				ttyout("^\b", tp);
			tp->t_state |= TS_LNCH;
			goto endcase;
		}
		if (c == tp->t_flushc) {
			if (t_flags&FLUSHO)
				tp->t_flags &= ~FLUSHO;
			else {
				ttyflush(tp, FWRITE);
				ttyecho(c, tp);
				if (tp->t_rawq.c_cc + tp->t_canq.c_cc)
					ttyretype(tp);
				tp->t_flags |= FLUSHO;
			}
			goto startoutput;
		}
		if (c == tp->t_suspc) {
			gsignal(tp->t_pgrp, SIGTSTP);
			if ((t_flags&NOFLSH) == 0)
				ttyflush(tp, FREAD);
			ttyecho(c, tp);
			goto endcase;
		}
	}

	if (!(tp->t_state&TS_LITCHR)) {

		/*
		 * Handle start/stop characters.
		 */
		if (c == tp->t_stopc) {
			if ((tp->t_state&TS_TTSTOP) == 0) {
				tp->t_state |= TS_TTSTOP;
				(*cdevsw[major(tp->t_dev)].d_stop)(tp, 0);
				return;
			}
			if (c != tp->t_startc)
				return;
			goto endcase;
		}
		if (c == tp->t_startc)
			goto restartoutput;

		/*
		 * Look for interrupt/quit chars.
		 */
		if (c == tp->t_intrc || c == tp->t_quitc) {
			gsignal(tp->t_pgrp, c == tp->t_intrc ? SIGINT : SIGQUIT);
			if ((t_flags&NOFLSH) == 0)
				ttyflush(tp, FREAD|FWRITE);
			ttyecho(c, tp);
			goto endcase;
		}
	}

	/*
	 * Cbreak mode, don't process line editing
	 * characters; check high water mark for wakeup.
	 */
	if (t_flags&CBREAK) {
		if (tp->t_rawq.c_cc > TTYHOG) {
			if (tp->t_outq.c_cc < TTHIWAT(tp) &&
			    tp->t_line == NTTYDISC)
				(void) ttyoutput(CTRL(g), tp);
		} else {

			/*
			 * if we need the top half to ignore the next
			 * char or LITCHAR is in the input stream, put
			 * an extra LITCHAR in the queue.
			 */
			ret = 1;
			if (tp->t_state&TS_LITCHR || c == LITCHAR) {
				ret = putc(LITCHAR, &tp->t_rawq);
				tp->t_rlitcount++;
				if (tp->t_rocount++ == 0)
					tp->t_rocol = tp->t_col;
			}

			if (ret >= 0 && putc(c, &tp->t_rawq) >= 0) {
				ttwakeup(tp);
				ttyecho(c, tp);
			}
			tp->t_state &= ~TS_LITCHR;
		}
		goto endcase;
	}

	if (tp->t_state&TS_LITCHR)
		goto inchar;

	/*
	 * From here on down cooked mode character
	 * processing takes place.
	 */
	if ((tp->t_state&TS_QUOT) &&
	    (c == tp->t_erase || c == tp->t_kill)) {
		ttyrub(unputc(&tp->t_rawq), tp);
		tp->t_state |= TS_LITCHR;
		goto inchar;
	}
	if (c == tp->t_erase) {
		if (tp->t_rawq.c_cc)
			ttyrub(unputc(&tp->t_rawq), tp);
		goto endcase;
	}
	if (c == tp->t_kill) {
		if (t_flags&CRTKIL &&
		    tp->t_rawq.c_cc == tp->t_rocount) {
			while (tp->t_rawq.c_cc)
				ttyrub(unputc(&tp->t_rawq), tp);
		} else {
			ttyecho(c, tp);
			ttyecho('\n', tp);
			while (getc(&tp->t_rawq) > 0)
				continue;
			tp->t_rocount = 0;
		}
		tp->t_state &= ~TS_LOCAL;
		goto endcase;
	}

	/*
	 * New line discipline,
	 * check word erase/reprint line.
	 */
	if (tp->t_line == NTTYDISC) {
		if (c == tp->t_werasc) {
			if (tp->t_rawq.c_cc == 0)
				goto endcase;
			do {
				c = unputc(&tp->t_rawq);
				if (c != ' ' && c != '\t')
					goto erasenb;
				ttyrub(c, tp);
			} while (tp->t_rawq.c_cc);
			goto endcase;
	erasenb:
			do {
				ttyrub(c, tp);
				if (tp->t_rawq.c_cc == 0)
					goto endcase;
				c = unputc(&tp->t_rawq);
			} while (c != ' ' && c != '\t');
			(void) putc(c, &tp->t_rawq);
			goto endcase;
		}
		if (c == tp->t_rprntc) {
			ttyretype(tp);
			goto endcase;
		}
	}

	/*
	 * Check for input buffer overflow
	 */
inchar:
	if (tp->t_rawq.c_cc+tp->t_canq.c_cc >= TTYHOG) {
		if (tp->t_line == NTTYDISC)
			(void) ttyoutput(CTRL(g), tp);
		goto endcase;
	}

	/*
	 * if we need the top half to ignore the next
	 * char or LITCHAR is in the input stream, put
	 * an extra LITCHAR in the queue.
	 */
	ret = 1;
	if (tp->t_state&TS_LITCHR || c == LITCHAR) {
		ret = putc(LITCHAR, &tp->t_rawq);
		tp->t_rlitcount++;
		if (tp->t_rocount++ == 0)
			tp->t_rocol = tp->t_col;
	}

	/*
	 * Put data char in q for user and
	 * wakeup on seeing a line delimiter.
	 */
	if (ret >= 0 && putc(c, &tp->t_rawq) >= 0) {
		if (ttbreakc(c, tp) && !(tp->t_state&TS_LITCHR)) {
			tp->t_rocount = 0;
			catq(&tp->t_rawq, &tp->t_canq);
			tp->t_clitcount += tp->t_rlitcount;
			tp->t_rlitcount = 0;
			ttwakeup(tp);
		} else if (tp->t_rocount++ == 0)
			tp->t_rocol = tp->t_col;
		tp->t_state &= ~TS_QUOT;
		if (c == '\\')
			tp->t_state |= TS_QUOT;
		if (tp->t_state&TS_ERASE) {
			tp->t_state &= ~TS_ERASE;
			(void) ttyoutput('/', tp);
		}
		i = tp->t_col;
		ttyecho(c, tp);
		if (c == tp->t_eofc && t_flags&ECHO
		    && !(tp->t_state&TS_LITCHR)) {
			i = MIN(2, tp->t_col - i);
			while (i > 0) {
				(void) ttyoutput('\b', tp);
				i--;
			}
		}
		tp->t_state &= ~TS_LITCHR;
	}

endcase:
	/*
	 * If DEC-style start/stop is enabled don't restart
	 * output until seeing the start character.
	 */
	if (t_flags&DECCTQ && tp->t_state&TS_TTSTOP &&
	    tp->t_startc != tp->t_stopc)
		return;

restartoutput:
	tp->t_state &= ~TS_TTSTOP;
	tp->t_flags &= ~FLUSHO;

startoutput:
	ttstart(tp);
}

/*
 * Put character on TTY output queue, adding delays,
 * expanding tabs, and handling the CR/NL bit.
 * This is called both from the top half for output,
 * and from interrupt level for echoing.
 * The arguments are the character and the tty structure.
 * Returns < 0 if putc succeeds, otherwise returns char to resend
 * Must be recursive.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttyoutput(c, tp)
	register c;
	register struct tty *tp;
{
	register char *colp;
	register ctype;

	if (tp->t_flags & (RAW|LITOUT)) {
		if (tp->t_flags&FLUSHO)
			return (-1);
		if (putc(c, &tp->t_outq))
			return (c&0377);
		l.cnt.v_ttyout++;
		return (-1);
	}

	/*
	 *  Don't mask the 8th bit in PASS8 mode.
	 */
	if (!(tp->t_flags&PASS8))
		c &= 0177;
	/*
	 * Ignore EOT in normal mode to avoid
	 * hanging up certain terminals.
	 */
	if (c == CEOT && (tp->t_flags&CBREAK) == 0)
		return (-1);
	/*
	 * Turn tabs to spaces as required
	 */
	if (c == '\t' && (tp->t_flags&TBDELAY) == XTABS) {
		c = 8 - (tp->t_col&7);
		if ((tp->t_flags&FLUSHO) == 0) {
			c -= b_to_q("        ", c, &tp->t_outq);
			l.cnt.v_ttyout += c;
		}
		tp->t_col += c;
		return (c ? -1 : '\t');
	}
	l.cnt.v_ttyout++;
	/*
	 * for upper-case-only terminals,
	 * generate escapes.
	 */
	if (tp->t_flags&LCASE) {
		colp = "({)}!|^~'`";
		while (*colp++)
			if (c == *colp++) {
				if (ttyoutput('\\', tp) >= 0)
					return (c&0377);
				c = colp[-2];
				break;
			}
		if ('A' <= c && c <= 'Z') {
			if (ttyoutput('\\', tp) >= 0)
				return (c&0377);
		} else if ('a' <= c && c <= 'z')
			c += 'A' - 'a';
	}

	/*
	 * turn <nl> to <cr><lf> if desired.
	 */
	if (c == '\n' && tp->t_flags&CRMOD)
		if (ttyoutput('\r', tp) >= 0)
			return (c&0377);
	if (c == '~' && tp->t_flags&TILDE)
		c = '`';
	if ((tp->t_flags&FLUSHO) == 0 && putc(c, &tp->t_outq))
		return (c&0377);
	/*
	 * Calculate delays.
	 * The numbers here represent clock ticks
	 * and are not necessarily optimal for all terminals.
	 * The delays are indicated by characters above 0200.
	 * In raw mode there are no delays and the
	 * transmission path is 8 bits wide.
	 *
	 * SHOULD JUST ALLOW USER TO SPECIFY DELAYS
	 */
	colp = &tp->t_col;
	ctype = partab[c];
	c = 0;
	switch (ctype&077) {

	case ORDINARY:
		(*colp)++;

	case CONTROL:
		break;

	case BACKSPACE:
		if (*colp)
			(*colp)--;
		break;

	/*
	 * This macro is close enough to the correct thing;
	 * it should be replaced by real user settable delays
	 * in any event...
	 */
	case NEWLINE:
		ctype = (tp->t_flags >> 8) & 03;
		if (ctype == 1) { /* tty 37 */
			if ((unsigned)*colp) {
				c = (((unsigned)*colp) >> 4) + 3;
				if ((unsigned)c > 6)
					c = 6;
			}
		} else if (ctype == 2) /* vt05 */
			c = mstohz(100);
		*colp = 0;
		break;

	case TAB:
		ctype = (tp->t_flags >> 10) & 03;
		if (ctype == 1) { /* tty 37 */
			c = 1 - (*colp | ~07);
			if (c < 5)
				c = 0;
		}
		*colp |= 07;
		(*colp)++;
		break;

	case VTAB:
		if (tp->t_flags&VTDELAY) /* tty 37 */
			c = 0177;
		break;

	case RETURN:
		ctype = (tp->t_flags >> 12) & 03;
		if (ctype == 1) /* tn 300 */
			c = mstohz(83);
		else if (ctype == 2) /* ti 700 */
			c = mstohz(166);
		else if (ctype == 3) { /* concept 100 */
			int i;

			if ((i = *colp) >= 0)
				for (; i < 9; i++)
					(void) putc(0177, &tp->t_outq);
		}
		*colp = 0;
	}
	if (c && (tp->t_flags&FLUSHO) == 0)
		(void) putc(c|0200, &tp->t_outq);
	return (-1);
}

/*
 * Called from device's read routine after it has
 * calculated the tty-structure given as argument.
 *
 * Assumes caller locked t_ttylock at SPLTTY.
 */
ttread(tp, uio)
	register struct tty *tp;
	struct uio *uio;
{
	register struct clist *qp;
	register c, t_flags;
	register caddr_t ibufp;
	int litmode, first, error = 0;
	short pgrp;		/* snapshot pgrp because of race with setpgrp */
	short must_uacc;	/* Must useracc each character */
	caddr_t uvaddr;
	int uvcnt, totcnt;
	struct iovec *uiovec;
	char ibuf[IBUFSIZ];

	totcnt = uio->uio_resid;
	ibufp = ibuf;
	uiovec = uio->uio_iov;
	uvaddr = uiovec->iov_base;
	uvcnt = uiovec->iov_len;
	must_uacc = 1;
	if (uvcnt && useracc(uvaddr, (u_int)uvcnt, B_WRITE))
		must_uacc = 0;

loop:
	/*
	 * Take any pending input first.
	 */
	if (tp->t_flags&PENDIN)
		ttypend(tp);

	if ((tp->t_state&TS_CARR_ON) == 0)
		return (EIO);

	/*
	 * Hang process if it's in the background.
	 */
	if (tp == u.u_ttyp && (((pgrp = u.u_procp->p_pgrp) != tp->t_pgrp) && pgrp)) {
		if ((u.u_procp->p_sigignore & sigmask(SIGTTIN)) ||
		    (u.u_procp->p_sigmask & sigmask(SIGTTIN)) ||
		     u.u_procp->p_flag&SVFORK)
			return (EIO);
		gsignal(pgrp, SIGTTIN);
		/*
		 * take the stop signal, longjmp if other signal present.
		 */
		v_lock(&tp->t_ttylock, SPL0);
		(void) p_lock(&u.u_procp->p_state, SPLHI);
		if (issig((lock_t *)NULL) > 0) {
			v_lock(&u.u_procp->p_state, SPL0);
			longjmp(&u.u_qsave);
		}
		v_lock(&u.u_procp->p_state, SPL0);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
		goto loop;
	}
	t_flags = tp->t_flags;

	/*
	 * In raw mode take characters directly from the
	 * raw queue w/o processing.
	 */
	if (t_flags&RAW) {
		if (tp->t_rawq.c_cc <= 0) {
			if (tp->t_state & TS_NBIO) {
				return (EWOULDBLOCK);
			}
			p_sema_v_lock(&tp->t_rawqwait, TTIPRI, &tp->t_ttylock,
					 SPL0);
			(void) p_lock(&tp->t_ttylock, SPLTTY);
			goto loop;
		}

 		while (tp->t_rawq.c_cc && totcnt) {
			/*
			 *  iovector empty? If so, get next one.
			 */
			while (uvcnt == 0) {
				++uiovec;
				uvaddr = uiovec->iov_base;
				uvcnt = uiovec->iov_len;
				must_uacc = 1;
				if (uvcnt && useracc(uvaddr, (u_int)uvcnt, B_WRITE))
					must_uacc = 0;
			}

			*ibufp++ = getc(&tp->t_rawq);
			totcnt--;

			if (must_uacc) {
				/*
				 * check if user address valid.
				 * Must do useracc on each char until
				 * offending address to maintain semantics.
				 */
				if (!useracc(uvaddr, 1, B_WRITE)) {
					/* error - bad user address */
					error = EFAULT;
					--ibufp;	/* back over bad */
					break;
				}
			}
			++uvaddr;
			--uvcnt;

			/*
			 * If ibuf full, write to user space.
			 */
			if (ibufp == &ibuf[IBUFSIZ]) {
				/* 
				 * drop lock since uiomove may block.
				 * usr space already verified for uiomove.
				 */
				v_lock(&tp->t_ttylock, SPL0);
				(void) uiomove(ibuf, IBUFSIZ, UIO_READ, uio);
				ibufp = ibuf;
				(void) p_lock(&tp->t_ttylock, SPLTTY);
#ifdef	TTYMON
				if (tp->t_vp)
					ttymon(tp, ibuf, IBUFSIZ);
#endif	TTYMON
			}
		}
		/*
		 * If any more to go, write to user space.
		 */
		if (ibufp != ibuf) {
			/* 
			 * drop lock since uiomove may block.
			 * usr space already verified for uiomove.
			 */
			v_lock(&tp->t_ttylock, SPL0);
			(void) uiomove(ibuf, (ibufp-ibuf), UIO_READ, uio);
			(void) p_lock(&tp->t_ttylock, SPLTTY);
#ifdef	TTYMON
			if (tp->t_vp)
				ttymon(tp, ibuf, (ibufp-ibuf));
#endif	TTYMON
		}
		goto checktandem;
	}

	/*
	 * In cbreak mode use the rawq, otherwise
	 * take characters from the canonicalized q.
	 */
	qp = t_flags&CBREAK ? &tp->t_rawq : &tp->t_canq;

	/*
	 * No input, sleep on rawq awaiting hardware
	 * receipt and notification.
	 */
	if (qp->c_cc <= 0) {
		if (tp->t_state & TS_NBIO) {
			return (EWOULDBLOCK);
		}
		p_sema_v_lock(&tp->t_rawqwait, TTIPRI, &tp->t_ttylock, SPL0);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
		goto loop;
	}

	/*
	 * Input present, perform input mapping
	 * and processing (we're not in raw mode).
	 */
	first = 1;
	litmode = 0;
	while ((c = getc(qp)) >= 0) {
		if (t_flags&CRMOD && c == '\r')
			c = '\n';
		else if (c == LITCHAR && !litmode) {
			litmode++;
			tp->t_clitcount--;
			continue;
		}

		/*
		 * handle the case where two LITCHARs are in the
		 * input stream.  (This will be true when a single
		 * LITCHAR gets input.)  If so, treat it as a single
		 * LICHAR.
		 */
		if (litmode) {
			if (c != LITCHAR)
				goto takechar;
			litmode = 0;
		}
		/*
		 * Hack lower case simulation on
		 * upper case only terminals.
		 */
		if (t_flags&LCASE && c <= 0177)
			if (tp->t_state&TS_BKSL) {
				if (maptab[c])
					c = maptab[c];
				tp->t_state &= ~TS_BKSL;
			} else if (c >= 'A' && c <= 'Z')
				c += 'a' - 'A';
			else if (c == '\\') {
				tp->t_state |= TS_BKSL;
				continue;
			}
		/*
		 * Check for delayed suspend character.
		 */
		if (tp->t_line == NTTYDISC && c == tp->t_dsuspc) {
			gsignal(tp->t_pgrp, SIGTSTP);
			if (first) {
				/*
				 * take the stop signal, longjmp if other signal
				 * present.
				 */
				v_lock(&tp->t_ttylock, SPL0);
				(void) p_lock(&u.u_procp->p_state, SPLHI);
				if (issig((lock_t *)NULL) > 0) {
					v_lock(&u.u_procp->p_state, SPL0);
					longjmp(&u.u_qsave);
				}
				v_lock(&u.u_procp->p_state, SPL0);
				(void) p_lock(&tp->t_ttylock, SPLTTY);
				goto loop;
			}
			break;
		}
		/*
		 * Interpret EOF only in cooked mode.
		 */
		if (c == tp->t_eofc && (t_flags&CBREAK) == 0)
			break;

		/*
		 *  iovector empty? If so, get next one.
		 */
takechar:
		while (uvcnt == 0) {
			++uiovec;
			uvaddr = uiovec->iov_base;
			uvcnt = uiovec->iov_len;
			must_uacc = 1;
			if (uvcnt && useracc(uvaddr, (u_int)uvcnt, B_WRITE))
				must_uacc = 0;
		}

 		*ibufp++ = (t_flags&PASS8) ? c : c & 0177;
		totcnt--;

		if (must_uacc) {
			/*
			 * check if user address valid.
			 * Must do useracc on each char until
			 * offending address to maintain semantics.
			 */
			 if (!useracc(uvaddr, 1, B_WRITE)) {
				/* error - bad user address */
				error = EFAULT;
				--ibufp;	/* back over offending char */
				break;
			}
		}
		++uvaddr;
		--uvcnt;

		/*
		 * Have we satisfied the request?
		 */
 		if (totcnt == 0)
			break;
		/*
		 * In cooked mode check for a "break character"
		 * marking the end of a "line of input".
		 */
		if ((t_flags&CBREAK) == 0 && ttbreakc(c, tp) && !litmode)
			break;
		litmode = 0;
		/*
		 * If ibuf full, write to user space.
		 */
		if (ibufp == &ibuf[IBUFSIZ]) {
			/* 
			 * drop lock since uiomove may block.
			 * usr space already verified for uiomove.
			 */
			v_lock(&tp->t_ttylock, SPL0);
			(void) uiomove(ibuf, IBUFSIZ, UIO_READ, uio);
			ibufp = ibuf;
			(void) p_lock(&tp->t_ttylock, SPLTTY);
#ifdef	TTYMON
			if (tp->t_vp)
				ttymon(tp, ibuf, IBUFSIZ);
#endif	TTYMON
		}
		first = 0;
	}
	/*
	 * If any more to go, write to user space.
	 */
	if (ibufp != ibuf) {
		/* 
		 * drop lock since uiomove may block.
		 * usr space already verified for uiomove.
		 */
		v_lock(&tp->t_ttylock, SPL0);
		(void) uiomove(ibuf, (ibufp-ibuf), UIO_READ, uio);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
#ifdef	TTYMON
		if (tp->t_vp)
			ttymon(tp, ibuf, (ibufp-ibuf));
#endif	TTYMON
	}
	tp->t_state &= ~TS_BKSL;

checktandem:
	/*
	 * Look to unblock output now that (presumably)
	 * the input queue has gone down.
	 */
	if (tp->t_state&TS_TBLOCK && tp->t_rawq.c_cc < TTYHOG/5)
		if (putc(tp->t_startc, &tp->t_outq) == 0) {
			tp->t_state &= ~TS_TBLOCK;
			ttstart(tp);
		}
	return (error);
}

/*
 * Called from the device's write routine after it has
 * calculated the tty-structure given as argument.
 *
 * Assumes caller locked t_ttylock at SPLTTY.
 */
ttwrite(tp, uio)
	register struct tty *tp;
	register struct uio *uio;
{
	register char *cp;
	register int cc, ce, c;
	int i, hiwat, cnt, error;
	short pgrp;		/* snapshot pgrp because of race with setpgrp */
	char obuf[OBUFSIZ];

	hiwat = TTHIWAT(tp);
	cnt = uio->uio_resid;
	error = 0;
loop:
	if ((tp->t_state&TS_CARR_ON) == 0)
		return (EIO);

	/*
	 * Hang the process if it's in the background.
	 */
	if (((pgrp = u.u_procp->p_pgrp) != tp->t_pgrp) && tp == u.u_ttyp &&
	      (tp->t_flags&TOSTOP) && (u.u_procp->p_flag&SVFORK)==0 &&
	      !(u.u_procp->p_sigignore & sigmask(SIGTTOU)) &&
	      !(u.u_procp->p_sigmask & sigmask(SIGTTOU))) {
		gsignal(pgrp, SIGTTOU);
		/*
		 * take the stop signal, longjmp if other signal present.
		 */
		v_lock(&tp->t_ttylock, SPL0);
		(void) p_lock(&u.u_procp->p_state, SPLHI);
		if (issig((lock_t *)NULL) > 0) {
			v_lock(&u.u_procp->p_state, SPL0);
			longjmp(&u.u_qsave);
		}
		v_lock(&u.u_procp->p_state, SPL0);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
		goto loop;
	}

	/*
	 * Process the user's data in at most OBUFSIZ
	 * chunks.  Perform lower case simulation and
	 * similar hacks.  Keep track of high water
	 * mark, sleep on overflow awaiting device aid
	 * in acquiring new space.
	 */
	while (uio->uio_resid > 0) {
		if (tp->t_outq.c_cc > hiwat) {
			cc = 0;
			goto ovhiwat;
		}

		/*
		 * Grab a hunk of data from the user.
		 */
		cc = uio->uio_iov->iov_len;
		if (cc == 0) {
			uio->uio_iovcnt--;
			uio->uio_iov++;
			if (uio->uio_iovcnt <= 0) {
				v_lock(&tp->t_ttylock, SPL0);
				panic("ttwrite");
				/*
				 *+ ttwrite() has been called to write
				 *+ characters to a tty, but the user
				 *+ I/O is zero.
				 */
			}
			continue;
		}
		if (cc > OBUFSIZ)
			cc = OBUFSIZ;
		cp = obuf;
		v_lock(&tp->t_ttylock, SPL0);
		error = uiomove(cp, cc, UIO_WRITE, uio);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
		/*
		 * We may have faulted and hence blocked. To be safe we
		 * need to recheck if carrier is still active.
		 */
		if ((tp->t_state & TS_CARR_ON) == 0) {
			v_lock(&tp->t_ttylock, SPL0);
			return (EIO);
		}
#ifdef	TTYMON
		if (tp->t_vp)
			ttymon(tp, cp, cc);
#endif	TTYMON
		if (error)
			break;
		if (tp->t_flags&FLUSHO)
			continue;
		/*
		 * If we're mapping lower case or kludging tildes,
		 * then we've got to look at each character, so
		 * just feed the stuff to ttyoutput...
		 */
		if (tp->t_flags & (LCASE|TILDE)) {
			while (cc > 0) {
				c = *cp++;
				tp->t_rocount = 0;
				if ((c = ttyoutput(c, tp)) >= 0) {
					/* out of clists, wait a bit */
					ttstart(tp);
					v_lock(&tp->t_ttylock, SPL0);
					p_sema(&lbolt, TTOPRI);
					(void) p_lock(&tp->t_ttylock, SPLTTY);
					tp->t_rocount = 0;
					if (cc != 0) {
						uio->uio_iov->iov_base -= cc;
						uio->uio_iov->iov_len += cc;
						uio->uio_resid += cc;
						uio->uio_offset -= cc;
					}
					goto loop;
				}
				--cc;
				if (tp->t_outq.c_cc > hiwat)
					goto ovhiwat;
			}
			continue;
		}
		/*
		 * If nothing fancy need be done, grab those characters we
		 * can handle without any of ttyoutput's processing and
		 * just transfer them to the output q.  For those chars
		 * which require special processing (as indicated by the
		 * bits in partab), call ttyoutput.  After processing
		 * a hunk of data, look for FLUSHO so ^O's will take effect
		 * immediately.
		 */
		while (cc > 0) {
			if (tp->t_flags & (RAW|LITOUT))
				ce = cc;
			else {
				ce = cc - scanc((unsigned)cc, (caddr_t)cp,
				   (caddr_t)partab, 077);
				/*
				 * If ce is zero, then we're processing
				 * a special character through ttyoutput.
				 */
				if (ce == 0) {
					tp->t_rocount = 0;
					if (ttyoutput(*cp, tp) >= 0) {
						/* no c-lists, wait a bit */
						ttstart(tp);
						v_lock(&tp->t_ttylock, SPL0);
						p_sema(&lbolt, TTOPRI);
						(void) p_lock(&tp->t_ttylock, SPLTTY);
						if (cc != 0) {
							uio->uio_iov->iov_base -= cc;
							uio->uio_iov->iov_len += cc;
							uio->uio_resid += cc;
							uio->uio_offset -= cc;
						}
						goto loop;
					}
					cp++, cc--;
					if (tp->t_flags&FLUSHO ||
					    tp->t_outq.c_cc > hiwat)
						goto ovhiwat;
					continue;
				}
			}
			/*
			 * A bunch of normal characters have been found,
			 * transfer them en masse to the output queue and
			 * continue processing at the top of the loop.
			 * If there are any further characters in this
			 * <= OBUFSIZ chunk, the first should be a character
			 * requiring special handling by ttyoutput.
			 */
			tp->t_rocount = 0;
			i = b_to_q(cp, ce, &tp->t_outq);
			ce -= i;
			tp->t_col += ce;
			cp += ce, cc -= ce, l.cnt.v_ttyout += ce;
			if (i > 0) {
				/* out of c-lists, wait a bit */
				ttstart(tp);
				v_lock(&tp->t_ttylock, SPL0);
				p_sema(&lbolt, TTOPRI);
				(void) p_lock(&tp->t_ttylock, SPLTTY);
				uio->uio_iov->iov_base -= cc;
				uio->uio_iov->iov_len += cc;
				uio->uio_resid += cc;
				uio->uio_offset -= cc;
				goto loop;
			}
			if (tp->t_flags&FLUSHO || tp->t_outq.c_cc > hiwat)
				goto ovhiwat;
		}
	}
	ttstart(tp);
	return (error);

ovhiwat:
	if (cc != 0) {
		uio->uio_iov->iov_base -= cc;
		uio->uio_iov->iov_len += cc;
		uio->uio_resid += cc;
		uio->uio_offset -= cc;
	}
	ttstart(tp);

	/*
	 * This can only occur if FLUSHO is set in t_flags
	 * or if ttstart/oproc is synchronous (or very fast).
	 */
	if (tp->t_outq.c_cc <= hiwat)
		goto loop;
	if (tp->t_state&TS_NBIO) {
		if (uio->uio_resid == cnt)
			return (EWOULDBLOCK);
		return (0);
	}
	tp->t_state |= TS_ASLEEP;
	p_sema_v_lock(&tp->t_outqwait, TTOPRI, &tp->t_ttylock, SPL0);
	(void) p_lock(&tp->t_ttylock, SPLTTY);
	goto loop;
}

/*
 * Rubout one character from the rawq of tp
 * as cleanly as possible.  Also remove the
 * LITCHAR at the end of the rawq, if appropriate.
 * Note that all callers of ttyrub() have already done an
 * unputc() of the character to be removed.  This should
 * never change or unputc's below will be wrong.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttyrub(c, tp)
	register c;
	register struct tty *tp;
{
	register char *cp;
	register int savecol;
	register char cc;
	int  litchar;
	char *nextc();

	if ((tp->t_flags&ECHO) == 0)
		return;
	tp->t_flags &= ~FLUSHO;
	c &= 0377;
	/*
	 * If last char in rawq is LITCHAR, we need to wipe it out
	 * as well.  Note that two LITCHARs mean a single literal LITCHAR.
	 */
	litchar = 0;
	cc = unputc(&tp->t_rawq);
	if ((int)cc != -1 && (cc & 0377) == LITCHAR) {
		cc = unputc(&tp->t_rawq);
		tp->t_rlitcount--;
		if ((int)cc != -1 && (cc & 0377) == LITCHAR) {
			(void)putc(LITCHAR, &tp->t_rawq);
		} else 
			litchar++;
	}
	if (cc >= 0)
		(void)putc(cc, &tp->t_rawq);

	if (tp->t_flags&CRTBS) {
		if (tp->t_rocount == 0) {
			/*
			 * Screwed by ttwrite; retype
			 */
			ttyretype(tp);
			return;
		}
		if ((tp->t_flags&PASS8) && (c&0200))
			ttyrubo(tp, 1);
		else if (litchar && ((c&0377) == '\t' || (c&0377) == '\n'))
			ttyrubo(tp, 2);
		else switch (partab[c&=0177]&0177) {

		case ORDINARY:
			if (tp->t_flags&LCASE && c >= 'A' && c <= 'Z')
				ttyrubo(tp, 2);
			else
				ttyrubo(tp, 1);
			break;

		case VTAB:
		case BACKSPACE:
		case CONTROL:
		case RETURN:
			if (tp->t_flags&CTLECH)
				ttyrubo(tp, 2);
			break;

		case TAB:
			if (tp->t_rocount < tp->t_rawq.c_cc) {
				ttyretype(tp);
				return;
			}
			savecol = tp->t_col;
			tp->t_state |= TS_CNTTB;
			tp->t_flags |= FLUSHO;
			tp->t_col = tp->t_rocol;
			cp = tp->t_rawq.c_cf;
			for (; cp; cp = nextc(&tp->t_rawq, cp))
				ttyecho(*cp, tp);
			tp->t_flags &= ~FLUSHO;
			tp->t_state &= ~TS_CNTTB;
			/*
			 * savecol will now be length of the tab
			 */
			savecol -= tp->t_col;
			tp->t_col += savecol;
			if (savecol > 8)
				savecol = 8;		/* overflow screw */
			while (--savecol >= 0)
				(void) ttyoutput('\b', tp);
			break;

		default:
			/*
			 * We are trying to erase a character that we do
			 * not know how to erase.  The panic that used to
			 * be here has been replaced with a diagnostic printf
			 * and a return since the system should not be 
			 * paniced for one stray character.  At some time, the
			 * cause of the bad character's presence in the clist
			 * should be found and fixed.  -dog, 11/5/90
			 */

			 /* panic("ttyrub"); */

			 CPRINTF("ttyrub: dev 0x%x unerasable char 0x%x\n",
				  tp->t_dev, partab[c&=0177]&0177);
			/*
			 * Rather than try to continue, let's just return.
			 * Leave tp lock as-is.
			 */
			return;
		}
	} else if (tp->t_flags&PRTERA) {
		if ((tp->t_state&TS_ERASE) == 0) {
			(void) ttyoutput('\\', tp);
			tp->t_state |= TS_ERASE;
		}
		ttyecho(c, tp);
	} else
		ttyecho(tp->t_erase, tp);
	tp->t_rocount--;
	/*
	 * if there was a LITCHAR which was removed, account for it as well.
	 */
	if (litchar)
		tp->t_rocount--;
}

/*
 * Crt back over cnt chars perhaps
 * erasing them.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttyrubo(tp, cnt)
	register struct tty *tp;
	int cnt;
{
	register char *rubostring = tp->t_flags&CRTERA ? "\b \b" : "\b";

	while (--cnt >= 0)
		ttyout(rubostring, tp);
}

/*
 * Reprint the rawq line.
 * We assume c_cc has already been checked.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttyretype(tp)
	register struct tty *tp;
{
	register char *cp;
	int litmode = 0;
	char *nextc();

	if (tp->t_rprntc != 0377)
		ttyecho(tp->t_rprntc, tp);
	(void) ttyoutput('\n', tp);
	for (cp = tp->t_canq.c_cf; cp; cp = nextc(&tp->t_canq, cp)) {
		if ((*cp&0377) == LITCHAR && !litmode) {
			litmode++;
			continue;
		} else if (litmode)
			litmode = 0;
		ttyecho(*cp, tp);
	}
	litmode = 0;
	for (cp = tp->t_rawq.c_cf; cp; cp = nextc(&tp->t_rawq, cp)) {
		if ((*cp&0377) == LITCHAR && !litmode) {
			litmode++;
			continue;
		} else if (litmode)
			litmode = 0;
		ttyecho(*cp, tp);
	}
	tp->t_state &= ~TS_ERASE;
	tp->t_rocount = tp->t_rawq.c_cc;
	tp->t_rocol = 0;
}

/*
 * Echo a typed character to the terminal
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttyecho(c, tp)
	register c;
	register struct tty *tp;
{

	if ((tp->t_state&TS_CNTTB) == 0)
		tp->t_flags &= ~FLUSHO;
	if ((tp->t_flags&ECHO) == 0)
		return;
	c &= 0377;
	if (tp->t_flags&RAW) {
		(void) ttyoutput(c, tp);
		return;
	}
	/*
	 * Avoids unwanted CTLECH processing for characters
	 * in the range 0x80 - 0x9f.  ttyoutput will mask high
	 * bit if not in PASS8 mode.
	 */
	if (c&0200) {
		(void) ttyoutput(c, tp);
		return;
	}
	if (c == '\r' && tp->t_flags&CRMOD)
		c = '\n';
	if (tp->t_flags&CTLECH) {
		if ((c&0177) <= 037 && c!='\t' && c!='\n' || (c&0177)==0177
		    || ((tp->t_state&TS_LITCHR) && (c=='\t' || c=='\n'))) {
			(void) ttyoutput('^', tp);
			c &= 0177;
			if (c == 0177)
				c = '?';
			else if (tp->t_flags&LCASE)
				c += 'a' - 1;
			else
				c += 'A' - 1;
		}
	}
	if ((tp->t_flags&LCASE) && (c >= 'A' && c <= 'Z'))
		c += 'a' - 'A';
	(void) ttyoutput(c&0177, tp);
}

/*
 * Is c a break char for tp?
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttbreakc(c, tp)
	register c;
	register struct tty *tp;
{
	return (c == '\n' || c == tp->t_eofc || c == tp->t_brkc ||
		c == '\r' && (tp->t_flags&CRMOD));
}

/*
 * send string cp to tp
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */
ttyout(cp, tp)
	register char *cp;
	register struct tty *tp;
{
	register char c;

	while (c = *cp++)
		(void) ttyoutput(c, tp);
}
/*
 * Awaken those waiting for input.
 *
 * Assumes caller has locked t_ttylock at SPLTTY.
 */

ttwakeup(tp)
	struct tty *tp;
{

	if (tp->t_rsel) {
		selwakeup(tp->t_rsel, tp->t_state&TS_RCOLL);
		tp->t_state &= ~TS_RCOLL;
		tp->t_rsel = (struct proc *)NULL;
	}
	if (tp->t_state & TS_ASYNC)
		gsignal(tp->t_pgrp, SIGIO); 
	vall_sema(&tp->t_rawqwait);
}

/*
 * Output char to tty; console putchar style.
 */
tputchar(c, tp)
	int c;
	struct tty *tp;
{
	register spl_t s_ipl;

	if (tp && (tp->t_state&TS_CARR_ON)) {
		s_ipl = p_lock(&tp->t_ttylock, SPLTTY);
		/*
		 * Now that lock is secure, check again...
		 */
		if ((tp->t_state & TS_CARR_ON) == 0) {
			v_lock(&tp->t_ttylock, s_ipl);
			return (-1);
		}
		if (c == '\n')
			(void) ttyoutput('\r', tp);
		(void) ttyoutput(c, tp);
		ttstart(tp);
		v_lock(&tp->t_ttylock, s_ipl);
		return (0);
	}
	return (-1);
}

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
static	char	rcsid[] = "$Header: tty_pty.c 2.13 1991/06/07 04:03:50 $";
#endif

/*
 * tty_pty.c
 * 	Pseudo-teletype Driver
 * 	(actually two drivers, requiring two entries in 'cdevsw')
 */

/* $Log: tty_pty.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/ioctl.h"
#include "../h/jioctl.h"
#include "../h/tty.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/buf.h"		/* for B_WRITE define */

#include "../machine/intctl.h"
#include "../machine/gate.h"

#define BUFSIZ 100		/* Chunk size iomoved to/from user */

/*
 * pts == /dev/tty[pP]?
 * ptc == /dev/pty[pP]?
 *
 * Parallel structures.
 */
struct	tty *pt_tty;
struct	pt_ioctl {
	int	pt_flags;
	struct	proc *pt_selr, *pt_selw;
	int	pt_send;
	int	pt_ucntl;	/* user command */
	sema_t	pt_cread;	/* controller blocked on read from slave */
	sema_t	pt_cwrite;	/* controller blocked on write to slave */
} *pt_ioctl;

int	npty;			/* number of pseudo terminals */

#ifdef	TTYPTYDEBUG
int	ptyttydebug = 0;
#endif	TTYPTYDEBUG

static	struct	tty *tp_next_pseudo; /* next available pseudo */
#define	GETPTY	minor(-1)	/* device that hints at next free pty */

#define	PF_RCOLL	0x01
#define	PF_WCOLL	0x02
#define	PF_NBIO		0x04
#define	PF_PKT		0x08		/* packet mode */
#define	PF_STOPPED	0x10		/* user told stopped */
#define	PF_REMOTE	0x20		/* remote and flow controlled input */
#define	PF_NOSTOP	0x40
#define	PF_UCNTL	0x80		/* user command protocol */

/*
 * ptyboot allocates memory for pt_ttys and
 * pt_ioctls and initializes appropriate locks
 * and semaphores.
 */
ptyboot(numpty)
	u_long	numpty;
{
	register struct tty *tp;
	register struct pt_ioctl *pti;

	npty = numpty;
	pt_tty = (struct tty *)calloc(sizeof(struct tty)*npty);
	pt_ioctl = (struct pt_ioctl *)calloc(sizeof(struct pt_ioctl)*npty);
	tp_next_pseudo = pt_tty;
	/*
	 * initialize tty and pty structures
	 */
	for (tp = pt_tty, pti = pt_ioctl; tp < &pt_tty[npty]; ++tp, ++pti) {
		ttyinit(tp, G_PTY);
		init_sema(&pti->pt_cread, 0, 0, G_PTY);
		init_sema(&pti->pt_cwrite, 0, 0, G_PTY);
	}
}

/*ARGSUSED*/
ptsopen(dev, flag)
	dev_t dev;
{
	register struct tty *tp;
	int retval;

	if (minor(dev) >= npty)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];
	(void) p_lock(&tp->t_ttylock, SPLTTY);
	tp->t_nopen++;
	while (tp->t_state & TS_LCLOSE) {
		tp->t_state |= TS_WOPEN;
		p_sema_v_lock(&tp->t_rawqwait, TTIPRI, &tp->t_ttylock, SPL0);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
	}
	if ((tp->t_state & TS_ISOPEN) == 0) {
		ttychars(tp);		/* Set up default chars */
		tp->t_ispeed = tp->t_ospeed = EXTB;
		tp->t_flags = 0;	/* No features (nor raw mode) */
	} else if (tp->t_state&TS_XCLUDE && u.u_uid != 0) {
		--tp->t_nopen;
		v_lock(&tp->t_ttylock, SPL0);
		return (EBUSY);
	}
	if (tp->t_oproc)			/* Ctrlr still around. */
		tp->t_state |= TS_CARR_ON;
	while ((tp->t_state & TS_CARR_ON) == 0) {
		tp->t_state |= TS_WOPEN;
		p_sema_v_lock(&tp->t_rawqwait, TTIPRI, &tp->t_ttylock, SPL0);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
	}
	if (retval = (*linesw[tp->t_line].l_open)(dev, tp))
		--tp->t_nopen;
	else
		ptcwakeup(tp, FREAD|FWRITE);
	v_lock(&tp->t_ttylock, SPL0);
	return (retval);
}

ptsclose(dev)
	dev_t dev;
{
	register struct tty *tp;
	spl_t s_ipl;		/* saved ipl */

	tp = &pt_tty[minor(dev)];
	s_ipl = p_lock(&tp->t_ttylock, SPLTTY);
	if (--tp->t_nopen > 0 || (tp->t_state & TS_LCLOSE)) {
		v_lock(&tp->t_ttylock, s_ipl);
		return;
	}
	tp->t_state |= TS_LCLOSE;
	(*linesw[tp->t_line].l_close)(tp);
	ttyclose(tp);
	/*
	 * Let controller know slave has done last close.
	 */
	ptcwakeup(tp, FREAD|FWRITE);
	v_lock(&tp->t_ttylock, s_ipl);
}

ptsread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	register struct pt_ioctl *pti = &pt_ioctl[minor(dev)];
	register caddr_t ibufp;
	register caddr_t uvaddr;
	register int uvcnt;
	int totcnt;
	struct iovec *uiovec;
	int error = 0;
	short pgrp;
	short must_uacc;	/* Must useracc each character */
	char ibuf[IBUFSIZ];

again:
	(void) p_lock(&tp->t_ttylock, SPLTTY);
	if (pti->pt_flags & PF_REMOTE) {

		if ((tp->t_state & TS_CARR_ON) == 0) {
			/*
			 * Controller side gone.
			 */
			v_lock(&tp->t_ttylock, SPL0);
			return (EIO);
		}

		if (tp == u.u_ttyp && (((pgrp = u.u_procp->p_pgrp) != tp->t_pgrp) && pgrp )) {
			if ((u.u_procp->p_sigignore & sigmask(SIGTTIN)) ||
			    (u.u_procp->p_sigmask & sigmask(SIGTTIN)) ||
			    u.u_procp->p_flag&SVFORK) {
				v_lock(&tp->t_ttylock, SPL0);
				return (EIO);
			}
			gsignal(pgrp, SIGTTIN);
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
			goto again;
		}

		if (tp->t_canq.c_cc == 0) {
			if (tp->t_state & TS_NBIO) {
				v_lock(&tp->t_ttylock, SPL0);
				return (EWOULDBLOCK);
			}
			p_sema_v_lock(&tp->t_rawqwait, TTIPRI, &tp->t_ttylock, SPL0);
			goto again;
		}

		totcnt = uio->uio_resid;
		ibufp = ibuf;
		uiovec = uio->uio_iov;
		uvaddr = uiovec->iov_base;
		uvcnt = uiovec->iov_len;
		must_uacc = 1;
		if (uvcnt && useracc(uvaddr, (u_int)uvcnt, B_WRITE))
			must_uacc = 0;

		while (tp->t_canq.c_cc > 1 && totcnt > 0) {
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

			*ibufp++ = getc(&tp->t_canq);
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
		}
		if (tp->t_canq.c_cc == 1)
			(void) getc(&tp->t_canq);
		if (tp->t_canq.c_cc) {
			v_lock(&tp->t_ttylock, SPL0);
			return (error);
		}
	} else
		if (tp->t_oproc)
			error = (*linesw[tp->t_line].l_read)(tp, uio);
	ptcwakeup(tp, FWRITE);
	v_lock(&tp->t_ttylock, SPL0);
	return (error);
}

/*
 * Write to pseudo-tty.
 * Wakeups of controlling tty will happen
 * indirectly, when tty driver calls ptsstart.
 */
ptswrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	spl_t s_ipl;
	int retval;

	tp = &pt_tty[minor(dev)];
	s_ipl = p_lock(&tp->t_ttylock, SPLTTY);
	if (tp->t_oproc == 0) {
		v_lock(&tp->t_ttylock, s_ipl);
		return (EIO);
	}

	retval = (*linesw[tp->t_line].l_write)(tp, uio);
	v_lock(&tp->t_ttylock, s_ipl);
	return (retval);
}

/*
 * Start output on pseudo-tty.
 *
 * Called with ttylock locked at SPLTTY.
 */
ptsstart(tp)
	struct tty *tp;
{
	register struct pt_ioctl *pti;

	if (tp->t_state & TS_TTSTOP)
		return;

	pti = &pt_ioctl[minor(tp->t_dev)];
	if (pti->pt_flags & PF_STOPPED) {
		pti->pt_flags &= ~PF_STOPPED;
		pti->pt_send = TIOCPKT_START;
	}
	ptcwakeup(tp, FREAD);
}

/*
 * ptsselect
 */
ptsselect(dev, rw)
	dev_t	dev;
	int	rw;
{
	register struct tty *tp = &pt_tty[minor(dev)];
	int	retval;
	spl_t	s_ipl;

	s_ipl = p_lock(&tp->t_ttylock, SPLTTY);
	retval = (*linesw[tp->t_line].l_select)(tp, rw);
	v_lock(&tp->t_ttylock, s_ipl);
	return (retval);
}

/*
 * ptcwakeup - wake up controller
 *
 * Called with ttylock locked at SPLTTY.
 */
ptcwakeup(tp, flag)
	struct tty *tp;
	int flag;
{
	struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];

	if (flag & FREAD) {
		if (pti->pt_selr) {
			selwakeup(pti->pt_selr, pti->pt_flags & PF_RCOLL);
			pti->pt_flags &= ~PF_RCOLL;
			pti->pt_selr = (struct proc *)NULL;
		} 
		vall_sema(&pti->pt_cread);	/* slave has data for controller */
	}
	if (flag & FWRITE) {
		/*
		 * If control tty asleep on write to slave,
		 * awaken it to send more data to slave.
		 * Should be cv_sema since only one open allowed.
		 * However since that process can fork, multiple processes
		 * could assume controller aspect.
		 */
		if (pti->pt_selw) {
			selwakeup(pti->pt_selw, pti->pt_flags & PF_WCOLL);
			pti->pt_flags &= ~PF_WCOLL;
			pti->pt_selw = (struct proc *)NULL;
		}
		vall_sema(&pti->pt_cwrite);
	}
}

/*ARGSUSED*/
ptcopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct tty *tp;
	struct pt_ioctl *pti;
	spl_t s_ipl;

	if (minor(dev) == GETPTY)
		return (0);
	if (minor(dev) >= npty)
		return (ENXIO);
	tp = &pt_tty[minor(dev)];
	s_ipl = p_lock(&tp->t_ttylock, SPLTTY);
	/*
	 * Only one open of controller side allowed.
	 */
	if (tp->t_oproc) {
		v_lock(&tp->t_ttylock, s_ipl);
		return (EIO);
	}
	tp->t_oproc = ptsstart;
	tp->t_state |= TS_CARR_ON;

	pti = &pt_ioctl[minor(dev)];
	pti->pt_flags = 0;
	pti->pt_send = 0;
	pti->pt_ucntl = 0;
	pti->pt_selr = (struct proc *)NULL;	/* Clear stale select data */
	pti->pt_selw = (struct proc *)NULL;	/* Clear stale select data */

	if (tp->t_state & TS_WOPEN)
		vall_sema(&tp->t_rawqwait);	/* cntrlr now active */
	v_lock(&tp->t_ttylock, s_ipl);
	return (0);
}

ptcclose(dev)
	dev_t dev;
{
	register struct tty *tp;
	spl_t s_ipl;

	if (minor(dev) == GETPTY)
		return;
	tp = &pt_tty[minor(dev)];
	s_ipl = p_lock(&tp->t_ttylock, SPLTTY);
	tp->t_state &= ~TS_CARR_ON;	/* virtual carrier gone */
	if (tp->t_state & TS_ISOPEN) {
		if ((tp->t_flags & NOHANG) == 0) {
			gsignal(tp->t_pgrp, SIGHUP);
			gsignal(tp->t_pgrp, SIGCONT);
			ttyflush(tp, FREAD|FWRITE);
		}
	}
	tp->t_oproc = 0;		/* mark closed */
	v_lock(&tp->t_ttylock, s_ipl);
}

ptcread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	struct pt_ioctl *pti;
	register caddr_t ibufp;
	register caddr_t uvaddr;
	register int uvcnt, totcnt;
	short must_uacc;		/* Must useracc each character */
	struct iovec *uiovec;
	int error = 0;
	char ibuf[IBUFSIZ];

	if (minor(dev) == GETPTY)
		return (EIO);
	tp = &pt_tty[minor(dev)];
	pti = &pt_ioctl[minor(dev)];

	/*
	 * We want to block until the slave is open and there is
	 * something to read. But if we lost the slave or we're NBIO,
	 * then return the appropriate error instead.
	 */
	for (;;) {
		(void) p_lock(&tp->t_ttylock, SPLTTY);
		if (tp->t_state & TS_ISOPEN) {
			if ((pti->pt_flags & PF_PKT) && pti->pt_send) {
				int c;

				c = pti->pt_send;
				v_lock(&tp->t_ttylock, SPL0);
				error = ureadc(c, uio);
				if (error)
					return (error);
				(void) p_lock(&tp->t_ttylock, SPLTTY);
				pti->pt_send = 0;
				v_lock(&tp->t_ttylock, SPL0);
				return (0);
			}
			if ((pti->pt_flags & PF_UCNTL) && pti->pt_ucntl) {
				int c;

				c = pti->pt_ucntl;
				v_lock(&tp->t_ttylock, SPL0);
				error = ureadc(c, uio);
				if (error)
					return (error);
				(void) p_lock(&tp->t_ttylock, SPLTTY);
				pti->pt_ucntl = 0;
				v_lock(&tp->t_ttylock, SPL0);
				return (0);
			}
			if (tp->t_outq.c_cc && ((tp->t_state & TS_TTSTOP) == 0))
				break;
		}
		if ((tp->t_state & TS_CARR_ON) == 0) {
			v_lock(&tp->t_ttylock, SPL0);
			return (EIO);
		}
		if (pti->pt_flags & PF_NBIO) {
			v_lock(&tp->t_ttylock, SPL0);
			return (EWOULDBLOCK);
		}
		/*
		 * wait for input from slave.
		 */
		p_sema_v_lock(&pti->pt_cread, TTIPRI, &tp->t_ttylock, SPL0);
	}
	if (pti->pt_flags & (PF_PKT|PF_UCNTL)) {
		/*
		 * We are either in PF_PKT mode or PF_UCNTL mode with no command
		 * present. So, just insert a zero into the stream (no command).
		 */
		v_lock(&tp->t_ttylock, SPL0);
		error = ureadc(0, uio);
		(void) p_lock(&tp->t_ttylock, SPLTTY);
	}

	totcnt = uio->uio_resid;
	ibufp = ibuf;
	uiovec = uio->uio_iov;
	uvaddr = uiovec->iov_base;
	uvcnt = uiovec->iov_len;
	must_uacc = 1;
	if (uvcnt && useracc(uvaddr, (u_int)uvcnt, B_WRITE))
		must_uacc = 0;

	while (tp->t_outq.c_cc && totcnt > 0 && error == 0) {
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

		*ibufp++ = getc(&tp->t_outq);
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
	}
	if (tp->t_outq.c_cc <= TTLOWAT(tp)) {
		if (tp->t_state&TS_ASLEEP) {
			tp->t_state &= ~TS_ASLEEP;
			vall_sema(&tp->t_outqwait);
		}
		if (tp->t_wsel) {
			selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
			tp->t_state &= ~TS_WCOLL;
			tp->t_wsel = (struct proc *)NULL;
		}
	}
	v_lock(&tp->t_ttylock, SPL0);
	return (error);
}

/*
 * Stop output.
 *
 * Assumes caller locked t_ttylock at SPLTTY.
 */
ptsstop(tp, flush)
	register struct tty *tp;
	int flush;
{
	struct pt_ioctl *pti = &pt_ioctl[minor(tp->t_dev)];
	int flag;

	/* note: FLUSHREAD and FLUSHWRITE already ok */
	if (flush == 0) {
		flush = TIOCPKT_STOP;
		pti->pt_flags |= PF_STOPPED;
	} else {
		pti->pt_flags &= ~PF_STOPPED;
	}
	pti->pt_send |= flush;

	/* change of perspective */
	flag = 0;
	if (flush & FREAD)
		flag |= FWRITE;
	if (flush & FWRITE)
		flag |= FREAD;
	ptcwakeup(tp, flag);
}

ptcselect(dev, rw)
	dev_t dev;
	int rw;
{
	register struct tty *tp;
	struct pt_ioctl *pti;
	spl_t s_ipl;

	if (minor(dev) == GETPTY)
		return (1);

	tp = &pt_tty[minor(dev)];
	pti = &pt_ioctl[minor(dev)];

	s_ipl = p_lock(&tp->t_ttylock, SPLTTY);
	if ((tp->t_state & TS_CARR_ON) == 0) {
		v_lock(&tp->t_ttylock, s_ipl);
		return (1);
	}

	switch (rw) {

	case FREAD:
		if ((tp->t_state & TS_ISOPEN) &&
		     tp->t_outq.c_cc && (tp->t_state & TS_TTSTOP) == 0) {
			v_lock(&tp->t_ttylock, s_ipl);
			return (1);
		}
		/* FALL THROUGH */

	case 0:						/* exceptional */
		if ((tp->t_state & TS_ISOPEN) &&
		    ((pti->pt_flags & PF_PKT) && pti->pt_send ||
		     (pti->pt_flags & PF_UCNTL) && pti->pt_ucntl)) {
			/* The command is available to be read */
			v_lock(&tp->t_ttylock, s_ipl);
			return (1);
		}
		if (pti->pt_selr) {
			/*
			 * If the current process is already known to be
			 * selecting, then nothing more to do. This can
			 * happen if pt_selr contains stale data from a
			 * previous call to select from this process.
			 * This avoids false collisions.
			 */
			if (pti->pt_selr == u.u_procp)
				break;

			/*
			 * If 1st selector is waiting on selwait or
			 * has not yet blocked on selwait, then we
			 * have a collision. It is sufficient to lock
			 * only the select_lck since selwakeup cannot
			 * happen without also locking select_lck.
			 */
			(void) p_lock(&select_lck, SPL6);
			if (pti->pt_selr->p_wchan == &selwait ||
			   (pti->pt_selr->p_flag & SSEL))
				pti->pt_flags |= PF_RCOLL;
			else
				pti->pt_selr = u.u_procp;
			v_lock(&select_lck, SPLTTY);
		} else
			pti->pt_selr = u.u_procp;
		break;

	case FWRITE:
		if (tp->t_state & TS_ISOPEN) {
			if (pti->pt_flags & PF_REMOTE) {
				if (tp->t_canq.c_cc == 0) {
					v_lock(&tp->t_ttylock, s_ipl);
					return (1);
				}
			} else {
				if (tp->t_rawq.c_cc + tp->t_canq.c_cc < TTYHOG-2) {
					v_lock(&tp->t_ttylock, s_ipl);
					return (1);
				}
				if (tp->t_canq.c_cc == 0 &&
				    ((tp->t_flags & (RAW|CBREAK)) == 0)) {
					v_lock(&tp->t_ttylock, s_ipl);
					return (1);
				}
			}
		}
		if (pti->pt_selw) {
			if (pti->pt_selw == u.u_procp)
				break;

			(void) p_lock(&select_lck, SPL6);
			if (pti->pt_selw->p_wchan == &selwait ||
			   (pti->pt_selw->p_flag & SSEL))
				pti->pt_flags |= PF_WCOLL;
			else
				pti->pt_selw = u.u_procp;
			v_lock(&select_lck, SPLTTY);
		} else
			pti->pt_selw = u.u_procp;
		break;
	}
	v_lock(&tp->t_ttylock, s_ipl);
	return (0);
}

ptcwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct tty *tp;
	register char *cp;
	register int cc = 0;
	register struct iovec *iov = NULL;
	int cnt = 0;
	struct pt_ioctl *pti;
	int error = 0;
	char locbuf[BUFSIZ];

	if (minor(dev) == GETPTY)
		return (EIO);
	tp = &pt_tty[minor(dev)];
	pti = &pt_ioctl[minor(dev)];

again:
	(void) p_lock(&tp->t_ttylock, SPLTTY);
	if ((tp->t_state & TS_ISOPEN) == 0)
		goto block;
	if (pti->pt_flags & PF_REMOTE) {
		if (tp->t_canq.c_cc)
			goto block;
		while (uio->uio_iovcnt > 0 && (tp->t_canq.c_cc < TTYHOG-1)) {
			iov = uio->uio_iov;
			if (iov->iov_len == 0) {
				uio->uio_iovcnt--;	
				uio->uio_iov++;
				continue;
			}
			if (cc == 0) {
				cc = MIN(iov->iov_len, BUFSIZ);
				cc = MIN(cc, TTYHOG - 1 - tp->t_canq.c_cc);
				cp = locbuf;
				v_lock(&tp->t_ttylock, SPL0);
				error = uiomove(cp, cc, UIO_WRITE, uio);
				if (error)
					return (error);
				/* Check again for safety */
				(void) p_lock(&tp->t_ttylock, SPLTTY);
				if ((tp->t_state & TS_ISOPEN) == 0) {
					/*
					 * Slave side went away, taking the
					 * carrier away with it. So error out.
					 */
					v_lock(&tp->t_ttylock, SPL0);
					return (EIO);
				}
			}
			if (cc)
				(void) b_to_q(cp, cc, &tp->t_canq);
			cc = 0;
		}
		(void) putc(0, &tp->t_canq);
		ttwakeup(tp);
		v_lock(&tp->t_ttylock, SPL0);
		return (0);
	}

	while (uio->uio_iovcnt > 0) {
		iov = uio->uio_iov;
		if (cc == 0) {
			if (iov->iov_len == 0) {
				uio->uio_iovcnt--;	
				uio->uio_iov++;
				continue;
			}
			cc = MIN(iov->iov_len, BUFSIZ);
			cp = locbuf;
			v_lock(&tp->t_ttylock, SPL0);
			error = uiomove(cp, cc, UIO_WRITE, uio);
			if (error)
				return (error);
			/* Check again for safety */
			(void) p_lock(&tp->t_ttylock, SPLTTY);
			if ((tp->t_state & TS_ISOPEN) == 0) {
				v_lock(&tp->t_ttylock, SPL0);
				return (EIO);
			}
		}
		while (cc > 0) {
			if ((tp->t_rawq.c_cc + tp->t_canq.c_cc) >= TTYHOG - 2 &&
			    ((tp->t_canq.c_cc > 0) ||
			     tp->t_flags & (RAW|CBREAK))) {
				vall_sema(&tp->t_rawqwait);
				goto block;
			}
			(*linesw[tp->t_line].l_rint)(*cp++, tp);
			cnt++;
			cc--;
		}
		cc = 0;
	}
	v_lock(&tp->t_ttylock, SPL0);
	return (0);

block:
	/*
	 * Come here to wait for slave to open, for space in outq, or
	 * space in rawq.
	 */
	if ((tp->t_state & TS_CARR_ON) == 0) {
		v_lock(&tp->t_ttylock, SPL0);
		return (EIO);
	}
	if (pti->pt_flags & PF_NBIO) {
		/* 
		 * We can only get here when cc and cnt are NULL so 
		 * print a debug message if this is somehow not the case.
		 */
		if (iov == NULL) {
#ifdef	TTYPTYDEBUG
			if ( ptyttydebug > 0 ) {
				printf("ptcwrite: iov is NULL\n");
				if (cc != 0 || cnt != 0) {
					printf("pcwrite: iov is NULL but cc is %d cnt is %d\n",
						cc,cnt);
				}
			}
#endif	/* TTYPTYDEBUG */
			v_lock(&tp->t_ttylock, SPL0);
			return (EWOULDBLOCK);
	
		}
		iov->iov_base -= cc;
		iov->iov_len += cc;
		uio->uio_resid += cc;
		uio->uio_offset -= cc;
		v_lock(&tp->t_ttylock, SPL0);
		if (cnt == 0)
			return (EWOULDBLOCK);
		return (0);
	}
	/*
	 * Block controller sending until slave ready.
	 */
	p_sema_v_lock(&pti->pt_cwrite, TTOPRI, &tp->t_ttylock, SPL0);
	goto again;
}

/*ARGSUSED*/
ptyioctl(dev, cmd, data, flag)
	caddr_t data;
	dev_t dev;
{
	register struct tty *tp;
	register struct pt_ioctl *pti;
	int error, stop;
	extern ttyinput();

	if (minor(dev) == GETPTY) {
		if (cmd == TIOCGETN) {
			ptcnext((int *) data);
			return (0);
		}
		return (EINVAL);
	}
	tp = &pt_tty[minor(dev)];
	pti = &pt_ioctl[minor(dev)];
	(void) p_lock(&tp->t_ttylock, SPLTTY);

	/*
	 * IF CONTROLLER STTY THEN MUST FLUSH TO PREVENT A HANG.
	 * ttywflush(tp) will hang if there are characters in the outq.
	 */
	if (cdevsw[major(dev)].d_open == ptcopen)
		switch (cmd) {

		case TIOCPKT:
			if (*(int *)data) {
				if (pti->pt_flags & PF_UCNTL) {
					v_lock(&tp->t_ttylock, SPL0);
					return (EINVAL);
				}
				pti->pt_flags |= PF_PKT;
			} else
				pti->pt_flags &= ~PF_PKT;
			v_lock(&tp->t_ttylock, SPL0);
			return (0);

		case TIOCUCNTL:
			if (*(int *)data) {
				if (pti->pt_flags & PF_PKT) {
					v_lock(&tp->t_ttylock, SPL0);
					return (EINVAL);
				}
				pti->pt_flags |= PF_UCNTL;
			} else
				pti->pt_flags &= ~PF_UCNTL;
			v_lock(&tp->t_ttylock, SPL0);
			return (0);

		case TIOCREMOTE:
			if (*(int *)data)
				pti->pt_flags |= PF_REMOTE;
			else
				pti->pt_flags &= ~PF_REMOTE;
			ttyflush(tp, FREAD|FWRITE);
			v_lock(&tp->t_ttylock, SPL0);
			return (0);

		case FIONBIO:
			if (*(int *)data)
				pti->pt_flags |= PF_NBIO;
			else
				pti->pt_flags &= ~PF_NBIO;
			v_lock(&tp->t_ttylock, SPL0);
			return (0);

		case OLD_TIOCSETP:
		case TIOCSETP:
		case OLD_TIOCSETN:
		case TIOCSETN:
		case TIOCSETD:
			while (getc(&tp->t_outq) >= 0)
				continue;
			/*
			 * Let the slave side know that we flushed
			 * the outq. The slave may be in ttywait().
			 */
			if (tp->t_state&TS_ASLEEP) {
				tp->t_state &= ~TS_ASLEEP;
				vall_sema(&tp->t_outqwait);
			}
			if (tp->t_wsel) {
				selwakeup(tp->t_wsel, tp->t_state & TS_WCOLL);
				tp->t_state &= ~TS_WCOLL;
				tp->t_wsel = (struct proc *)NULL;
			}
			break;

		}

	error = ttioctl(tp, cmd, data, flag);
	/*
	 * Since we use the tty queues internally,
	 * pty's can't be switched to disciplines which overwrite
	 * the queues.  We can't tell anything about the discipline
	 * from here...
	 */
	if (linesw[tp->t_line].l_rint != ttyinput) {
		(*linesw[tp->t_line].l_close)(tp);
		tp->t_line = 0;
		(void)(*linesw[tp->t_line].l_open)(dev, tp);
		error = ENOTTY;
	}
	if (error < 0) {
		if ((pti->pt_flags & PF_UCNTL) && (cmd & ~0xff) == _IO(u,0)) {
			if (cmd & 0xff) {
				pti->pt_ucntl = (u_char) cmd;
				ptcwakeup(tp, FREAD);
			}
			v_lock(&tp->t_ttylock, SPL0);
			return (0);
		}
		error = ENOTTY;
	}
	stop = (tp->t_flags & RAW) == 0 &&
		tp->t_stopc == CTRL(s) && tp->t_startc == CTRL(q);
	if (pti->pt_flags & PF_NOSTOP) {
		if (stop) {
			pti->pt_send &= ~TIOCPKT_NOSTOP;
			pti->pt_send |= TIOCPKT_DOSTOP;
			pti->pt_flags &= ~PF_NOSTOP;
			ptcwakeup(tp, FREAD);
		}
	} else {
		if (!stop) {
			pti->pt_send &= ~TIOCPKT_DOSTOP;
			pti->pt_send |= TIOCPKT_NOSTOP;
			pti->pt_flags |= PF_NOSTOP;
			ptcwakeup(tp, FREAD);
		}
	}
	v_lock(&tp->t_ttylock, SPL0);
	return (error);
}

/*
 * ptcnext()
 *	Return the index of next available pseudo terminal.
 *
 * This is should be considered only a hint as we race with
 * anbody else doing the same thing.
 */

static
ptcnext(data)
	int	*data;
{
	register struct tty *tp;
	register struct tty *tp_limit;
	register int limit = *data;

	if ((unsigned)limit > npty)
		limit = npty;
	tp_limit = &pt_tty[limit];
	for (tp = tp_next_pseudo; limit > 0; --limit, ++tp) {
		if (tp >= tp_limit)
			tp = pt_tty;
		if (tp->t_oproc == 0) {
			tp_next_pseudo = tp;
			*data = tp - pt_tty;
			return;
		}
	}
	*data = -1;
}

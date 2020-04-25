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
static	char	rcsid[] = "$Header: sys_generic.c 2.8 1991/05/28 23:23:47 $";
#endif

/*
 * sys_generic.c
 *	File-type independent IO system-calls.
 */

/* $Log: sys_generic.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/ioctl.h"
#include "../h/file.h"
#include "../h/proc.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/stat.h"
#include "../h/vnode.h"
#include "../h/buf.h"

#include "../machine/gate.h"
#include "../machine/intctl.h"

/*
 * Read system call.
 */
read()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		unsigned count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	aiov.iov_base = (caddr_t)uap->cbuf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	rwuio(&auio, UIO_READ);
}

readv()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];

	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_READ);
}

/*
 * Write system call
 */
write()
{
	register struct a {
		int	fdes;
		char	*cbuf;
		int	count;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov;

	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	aiov.iov_base = uap->cbuf;
	aiov.iov_len = uap->count;
	rwuio(&auio, UIO_WRITE);
}

writev()
{
	register struct a {
		int	fdes;
		struct	iovec *iovp;
		int	iovcnt;
	} *uap = (struct a *)u.u_ap;
	struct uio auio;
	struct iovec aiov[16];

	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		u.u_error = EINVAL;
		return;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	u.u_error = copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	    (unsigned)(uap->iovcnt * sizeof (struct iovec)));
	if (u.u_error)
		return;
	rwuio(&auio, UIO_WRITE);
}

rwuio(uio, rw)
	register struct uio *uio;
	enum uio_rw rw;
{
	struct a {
		int	fdes;
	};
	register struct file *fp;
	register struct iovec *iov;
	int	i, count;

	GETF(fp, ((struct a *)u.u_ap)->fdes);
	if ((fp->f_flag&(rw==UIO_READ ? FREAD : FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	uio->uio_resid = 0;
	uio->uio_segflg = UIOSEG_USER;
	iov = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iov->iov_len < 0) {
			u.u_error = EINVAL;
			return;
		}
		uio->uio_resid += iov->iov_len;
		if (uio->uio_resid < 0) {
			u.u_error = EINVAL;
			return;
		}
		iov++;
	}
	/*
	 * Below sampling of f_offset and later increment races with
	 * other users of same fp even on mono-P, so no locking necessary
	 * If accurate "log" file is needed, use FAPPEND.
	 */
	count = uio->uio_resid;
	uio->uio_offset = fp->f_offset;
	if ((u.u_procp->p_flag&SOUSIG) == 0 && setjmp(&u.u_qsave)) {
		if (uio->uio_resid == count)
			u.u_eosys = RESTARTSYS;
	} else
		u.u_error = (*fp->f_ops->fo_rw)(fp, rw, uio);
	u.u_r.r_val1 = count - uio->uio_resid;
	u.u_ioch += u.u_r.r_val1;
	fp->f_offset += u.u_r.r_val1;
}

/*
 * Ioctl system call
 */
ioctl()
{
	register struct file *fp;
	struct a {
		int	fdes;
		int	cmd;
		caddr_t	cmarg;
	} *uap = (struct a *) u.u_ap;
	register int com;
	register u_int size;
	char data[IOCPARM_MASK+1];

	if ((fp = getf(uap->fdes)) == NULL)
		return;
	if ((fp->f_flag & (FREAD|FWRITE)) == 0) {
		u.u_error = EBADF;
		return;
	}
	com = uap->cmd;

	if (com == FIOCLEX) {
		u.u_error = ofile_set_flags(u.u_ofile_tab, uap->fdes, UF_EXCLOSE);
		return;
	}
	if (com == FIONCLEX) {
		u.u_error = ofile_set_flags(u.u_ofile_tab, uap->fdes, 0);
		return;
	}

	/*
	 * Interpret high order word to find
	 * amount of data to be copied to/from the
	 * user's address space.
	 */
	size = IOCPARM_LEN(com);
	if (size > sizeof (data)) {
		u.u_error = EFAULT;
		return;
	}
	if (com&IOC_IN) {
		if (size) {
			/*
			 * Allow NULL to be zero instead of a fault.
			 */
			if (size <= sizeof(int) && uap->cmarg == 0) {
				*(int *)data = 0;
			} else {
				u.u_error = copyin(uap->cmarg, (caddr_t)data,
							(u_int)size);
				if (u.u_error)
					return;
			}
		} else
			*(caddr_t *)data = uap->cmarg;
	} else if ((com&IOC_OUT) && size)
		/*
		 * Zero the buffer on the stack so the user
		 * always gets back something deterministic.
		 */
		bzero((caddr_t)data, size);
	else if (com&IOC_VOID)
		*(caddr_t *)data = uap->cmarg;

	switch (com) {

	case FIONBIO:
		u.u_error = fset(fp, FNDELAY, *(int *)data);
		return;

	case FIOASYNC:
		u.u_error = fset(fp, FASYNC, *(int *)data);
		return;

	case FIOSETOWN:
		u.u_error = fsetown(fp, *(int *)data);
		return;

	case FIOGETOWN:
		u.u_error = fgetown(fp, (int *)data);
		return;
	}
	u.u_error = (*fp->f_ops->fo_ioctl)(fp, com, data);
	/*
	 * Copy any data to user, size was
	 * already set and checked above.
	 */
	if (u.u_error == 0 && (com&IOC_OUT) && size)
		u.u_error = copyout(data, uap->cmarg, (u_int)size);
}

int	unselect();
int	nselcoll;	/* gated via select_lck */

/*
 * Select system call.
 */

select()
{
	register struct uap  {
		int	nd;
		long	*in, *ou, *ex;
		struct	timeval *tv;
	} *uap = (struct uap *)u.u_ap;
	register struct proc *p = u.u_procp;
	register spl_t	s_ipl;
	struct	timeval	atv;
	struct	buf	*bp = NULL;
	int	ncoll, ni;
	label_t	lqsave;
	fd_set	*ibits[3], *obits[3];
	fd_set	libits[3], lobits[3];

	if (uap->nd > OFILE_NOFILE(u.u_ofile_tab))
		uap->nd = OFILE_NOFILE(u.u_ofile_tab);	/* forgiving, if slightly wrong */

	/*
	 * If interested in <= FD_SETSIZE descriptors, use fd_set's on stack.
	 * Else get an empty buffer and use this (could use heap).  Need
	 * to have pointers to memory that won't move due to swapping.
	 * This awkwardness is necessary to allow unbounded number of fd's.
	 * FD_SETSIZE is chosen to accomodate most needs (see h/types.h)
	 * without overflowing.  setdtablesize() insures 6 such fd_set's
	 * can fit in a buffer.
	 */

	ni = howmany(uap->nd, NFDBITS);
	if (uap->nd <= FD_SETSIZE) {
		ibits[0] = &libits[0]; ibits[1] = &libits[1]; ibits[2] = &libits[2];
		obits[0] = &lobits[0]; obits[1] = &lobits[1]; obits[2] = &lobits[2];
	} else {
		int	fd_set_sz = FD_SET_SZ(uap->nd);	/* size in bytes */
		bp = geteblk(6 * fd_set_sz);
		ibits[0] = (fd_set *) (bp->b_un.b_addr + 0 * fd_set_sz);
		ibits[1] = (fd_set *) (bp->b_un.b_addr + 1 * fd_set_sz);
		ibits[2] = (fd_set *) (bp->b_un.b_addr + 2 * fd_set_sz);
		obits[0] = (fd_set *) (bp->b_un.b_addr + 3 * fd_set_sz);
		obits[1] = (fd_set *) (bp->b_un.b_addr + 4 * fd_set_sz);
		obits[2] = (fd_set *) (bp->b_un.b_addr + 5 * fd_set_sz);
	}

#define	getbits(name, x) \
	if (uap->name) { \
		u.u_error = copyin((caddr_t)uap->name, (caddr_t)ibits[x], \
					(unsigned)(ni * sizeof(fd_mask))); \
		if (u.u_error) \
			goto done; \
	}
	getbits(in, 0);
	getbits(ou, 1);
	getbits(ex, 2);
#undef	getbits

	if (uap->tv) {
		u.u_error = copyin((caddr_t)uap->tv,(caddr_t)&atv,sizeof(atv));
		if (u.u_error)
			goto done;
		if (itimerfix(&atv)) {
			u.u_error = EINVAL;
			goto done;
		}
		P_GATE(G_TIME, s_ipl);
		timevaladd(&atv, &time);
		V_GATE(G_TIME, s_ipl);
	}

	/*
	 * Loop until something selects "true" or timeout.
	 */

	for (;;) {
		ncoll = nselcoll;		/* no need to mutex on read */

		s_ipl = p_lock(&p->p_state, SPLHI);
		p->p_flag |= SSEL;
		v_lock(&p->p_state, s_ipl);

		/*
		 * Scan appropriate sets of fd's.  If find something, done.
		 * u_r.r_val1 was zeroed in syscall entry.
		 */

		if (uap->in)
			u.u_r.r_val1 = selscan(ibits[0], obits[0],
							FREAD, uap->nd);
		if (u.u_error == 0 && uap->ou)
			u.u_r.r_val1 += selscan(ibits[1], obits[1],
							FWRITE, uap->nd);
		if (u.u_error == 0 && uap->ex)
			u.u_r.r_val1 += selscan(ibits[2], obits[2],
							0, uap->nd);

		if (u.u_error || u.u_r.r_val1)
			break;				/* done */

		/*
		 * If the wakeup occured while selscan was executing, then
		 * the SSEL bit will have been cleared and we must retry.
		 * Also, if there was a selwakeup with collsion then we also
		 * retry. Collisions should be rare since more than one process
		 * has to be selecting on a particular (minor) device.
		 *
		 * Locking select_lck prevents any select wakeups or timeouts
		 * before the process sleeps on the selwait semaphore.
		 */

		s_ipl = p_lock(&select_lck, SPL6);

		if ((p->p_flag & SSEL) == 0 || nselcoll != ncoll) {
			v_lock(&select_lck, s_ipl);
			continue;
		}

		(void) p_lock(&p->p_state, SPLHI);
		p->p_flag &= ~SSEL;
		v_lock(&p->p_state, SPL6);

		if (uap->tv) {
			lqsave = u.u_qsave;
			if (setjmp(&u.u_qsave)) {
				untimeout(unselect, (caddr_t)p);
				u.u_error = EINTR;
				break;
			}
			VOID_P_GATE(G_TIME);
			/* this should be timercmp(&time, &atv, >=) */
			if (time.tv_sec > atv.tv_sec || time.tv_sec == atv.tv_sec &&
			    time.tv_usec >= atv.tv_usec) {
				V_GATE(G_TIME, SPL6);
				v_lock(&select_lck, s_ipl);
				break;
			}
			ltimeout(unselect, (caddr_t)p, hzto(&atv));
			V_GATE(G_TIME, SPL6);
		}

		p_sema_v_lock(&selwait, PZERO+1, &select_lck, s_ipl);

		if (uap->tv) {
			u.u_qsave = lqsave;
			untimeout(unselect, (caddr_t)p);
		}
	}
#define	putbits(name, x) \
	if (uap->name && u.u_error == 0) { \
		u.u_error = copyout((caddr_t)obits[x], (caddr_t)uap->name, \
					(unsigned)(ni * sizeof(fd_mask))); \
	}
	putbits(in, 0);
	putbits(ou, 1);
	putbits(ex, 2);
#undef putbits
done:
	if (bp)
		brelse(bp);
}

static
unselect(p)
	register struct proc *p;
{
	spl_t s_ipl;

	s_ipl = p_lock(&select_lck, SPL6);
	(void) p_lock(&p->p_state, SPLHI);

	if (p->p_stat == SSLEEP) {
		force_v_sema(p);	/* not signal, but p_sema can handle */
	}
	v_lock(&p->p_state, SPL6);
	v_lock(&select_lck, s_ipl);
}

static
selscan(ibits, obits, flag, nfd)
	fd_set	*ibits;
	fd_set	*obits;
{
	register fd_mask bits;
	register int j;
	register int i;
	register struct	file *fp;
	register struct proc *p = u.u_procp;
	int	n = 0;
	spl_t	s;

	bzero((caddr_t) obits->fds_bits, (u_int) FD_SET_SZ(nfd));

	for (i = 0; i < nfd; i += NFDBITS) {
		bits = ibits->fds_bits[i/NFDBITS];
		while ((j = ffs((int)bits)) && (i + --j) < nfd) {
			bits &= ~(1 << j);
			GETFP(fp, i + j);
			if (fp == NULL) {
				u.u_error = EBADF;
				if (p->p_flag & SSEL) {
					s = p_lock(&p->p_state, SPLHI);
					p->p_flag &= ~SSEL;
					v_lock(&p->p_state, s);
				}
				return(0);
			}
			if ((*fp->f_ops->fo_select)(fp, flag)) {
				FD_SET(i + j, obits);
				n++;
				if (p->p_flag & SSEL) {
					/*
					 * Have found one ready.
					 */
					s = p_lock(&p->p_state, SPLHI);
					p->p_flag &= ~SSEL;
					v_lock(&p->p_state, s);
				}
			}
			/*
			 * If the getf() above bump'd fp's ref-count, undo.
			 * This minor ugly unwinds shared ofile table
			 * referencing.
			 */
			if (u.u_fpref) {
				deref_file(u.u_fpref);
				u.u_fpref = NULL;
			}
		}
	}
	return(n);
}

/*ARGSUSED*/
seltrue(dev, flag)
	dev_t dev;
	int flag;
{

	return (1);
}

selwakeup(p, coll)
	register struct proc *p;
	int coll;
{
	spl_t s_ipl;

	s_ipl = p_lock(&select_lck, SPL6);
	if (coll) {
		/*
		 * If collision wakeup the world, since the
		 * processes that collided are not known to driver.
		 */
		nselcoll++;
		vall_sema(&selwait);
		v_lock(&select_lck, s_ipl);
		return;
	}
	/*
	 * need to lock process state to look at p_wchan
	 * and to write p_flag.
	 */
	(void) p_lock(&p->p_state, SPLHI);
	if (p->p_wchan == &selwait) {
		force_v_sema(p);
	}
	else if (p->p_flag & SSEL) {
		p->p_flag &= ~SSEL;
	}
	v_lock(&p->p_state, SPL6);
	v_lock(&select_lck, s_ipl);
}

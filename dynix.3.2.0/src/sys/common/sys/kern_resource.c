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
static	char	rcsid[] = "$Header: kern_resource.c 2.5 1991/05/29 00:20:39 $";
#endif

/*
 * kern_resource.c
 * 	Resource controls and accounting.
 */

/* $Log: kern_resource.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/uio.h"
#include "../h/vm.h"
#include "../h/kernel.h"

#include "../machine/intctl.h"

getpriority()
{
	register struct a {
		int	which;
		int	who;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;
	register int lnice;		/* local copy of p_nice */

	u.u_r.r_val1 = NZERO+20;
	u.u_error = ESRCH;
	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			u.u_r.r_val1 = u.u_procp->p_nice;
		else {
			p = pfind(uap->who);
			if (p == 0)
				return;
			u.u_r.r_val1 = p->p_nice;
			v_lock(&p->p_state, SPL0);
		}
		u.u_error = 0;
		break;

	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u.u_procp->p_pgrp;
		(void) p_lock(&proc_list, SPL6);
		for (p = proc; p < procmax; p++) {
			if (p->p_stat == NULL)
				continue;
			/*
			 * copy p_nice in case of concurrent setpriority.
			 * no problem with concurrent exit/newproc with
			 * proc_list locked.
			 */
			if (p->p_pgrp == uap->who &&
			    (lnice = p->p_nice) < u.u_r.r_val1) {
				u.u_r.r_val1 = lnice;
				u.u_error = 0;
			}
		}
		v_lock(&proc_list, SPL0);
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u.u_uid;
		(void) p_lock(&proc_list, SPL6);
		for (p = proc; p < procmax; p++) {
			if (p->p_stat == NULL)
				continue;
			if (p->p_uid == uap->who &&
			    (lnice = p->p_nice) < u.u_r.r_val1) {
				u.u_r.r_val1 = lnice;
				u.u_error = 0;
			}
		}
		v_lock(&proc_list, SPL0);
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
	u.u_r.r_val1 -= NZERO;
}

setpriority()
{
	register struct a {
		int	which;
		int	who;
		int	prio;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	u.u_error = ESRCH;
	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			donice(u.u_procp, uap->prio);
		else {
			p = pfind(uap->who);
			if (p == 0)
				return;
			donice(p, uap->prio);
			v_lock(&p->p_state, SPL0);
		}
		break;

	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u.u_procp->p_pgrp;
		/*
		 * No need to lock state, if races with
		 * setpgrp don't care. Cannot race with
		 * exit/newproc since proc_list is held.
		 */
		(void) p_lock(&proc_list, SPL6);
		for (p = proc; p < procmax; p++)
			if (p->p_pgrp == uap->who)
				donice(p, uap->prio);
		v_lock(&proc_list, SPL0);
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u.u_uid;
		(void) p_lock(&proc_list, SPL6);
		for (p = proc; p < procmax; p++)
			if (p->p_uid == uap->who)
				donice(p, uap->prio);
		v_lock(&proc_list, SPL0);
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
}

donice(p, n)
	register struct proc *p;
	register int n;
{

	if (u.u_uid && u.u_ruid &&
	    u.u_uid != p->p_uid && u.u_ruid != p->p_uid) {
		u.u_error = EACCES;
		return;
	}
	n += NZERO;
	if (n >= 2*NZERO)
		n = 2*NZERO - 1;
	if (n < 0)
		n = 0;
	if (n < p->p_nice && !suser()) {
		u.u_error = EACCES;
		return;
	}
	p->p_nice = n;
	if (u.u_error == ESRCH)
		u.u_error = 0;
}

setrlimit()
{
	register struct a {
		u_int	which;
		struct	rlimit *lim;
	} *uap = (struct a *)u.u_ap;
	struct rlimit alim;
	register struct rlimit *alimp;


	if (uap->which >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return;
	}
	alimp = &u.u_rlimit[uap->which];
	u.u_error = copyin((caddr_t)uap->lim, (caddr_t)&alim,
		sizeof (struct rlimit));
	if (u.u_error)
		return;
	if (alim.rlim_cur < 0 || alim.rlim_max < 0) {
		u.u_error = EINVAL;
		return;
	}
	if (alim.rlim_cur > alimp->rlim_max || alim.rlim_max > alimp->rlim_max)
		if (!suser())
			return;
	if (alim.rlim_cur > alim.rlim_max)
		alim.rlim_cur = alim.rlim_max;

	switch (uap->which) {
	case RLIMIT_DATA:
		if (alim.rlim_cur > ctob(MAXDSIZ))
			alim.rlim_cur = ctob(MAXDSIZ);
		break;

	case RLIMIT_STACK:
		if (alim.rlim_cur > ctob(MAXSSIZ))
			alim.rlim_cur = ctob(MAXSSIZ);
		break;

	case RLIMIT_RSS:
		u.u_procp->p_maxrss = alim.rlim_cur/CLBYTES;
		if (u.u_procp->p_maxrss < u.u_procp->p_rscurr)
			vsetRS((long) u.u_procp->p_maxrss);
		break;
	}
	*alimp = alim;
}

getrlimit()
{
	register struct a {
		u_int	which;
		struct	rlimit *rlp;
	} *uap = (struct a *)u.u_ap;

	if (uap->which >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = copyout((caddr_t)&u.u_rlimit[uap->which], (caddr_t)uap->rlp,
	    sizeof (struct rlimit));
}

getrusage()
{
	register struct a {
		int	who;
		struct	rusage *rusage;
	} *uap = (struct a *)u.u_ap;
	struct timeval stime;		/* avoid clock from changing ru_stime */
	spl_t s_ipl;			/* saved ipl */

	switch (uap->who) {

	case RUSAGE_SELF:
		/*
		 * Avoid possible 1 second error by collecting system
		 * time into local and then copy it out. Error can occur
		 * if clock interrupt occurs between the copyout of timeval
		 * vector elements.
		 */
		s_ipl = splhi();
		stime = u.u_ru.ru_stime;
		splx(s_ipl);
		u.u_error = copyout((caddr_t)&u.u_ru, (caddr_t)uap->rusage,
		    sizeof (struct rusage));
		if (u.u_error == 0)
			u.u_error = copyout((caddr_t)&stime,
				(caddr_t)&uap->rusage->ru_stime, sizeof (struct timeval));
		break;

	case RUSAGE_CHILDREN:
		u.u_error = copyout((caddr_t)&u.u_cru, (caddr_t)uap->rusage,
		    sizeof (struct rusage));
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
}

ruadd(ru, ru2)
	register struct rusage *ru;
	struct rusage *ru2;
{
	register long *ip;
	register long *ip2;

	timevaladd(&ru->ru_utime, &ru2->ru_utime);
	timevaladd(&ru->ru_stime, &ru2->ru_stime);
	if (ru->ru_maxrss < ru2->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	ip = &ru->ru_first; ip2 = &ru2->ru_first;
	for (; ip <= &ru->ru_last; ip++, ip2++)
		*ip += *ip2;
}

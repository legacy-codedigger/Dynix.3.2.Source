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
static	char	rcsid[] = "$Header: sys_tmp.c 2.11 1991/07/15 18:29:25 $";
#endif

/*
 * sys_tmp.c
 * 	Sequent specific system calls.
 */

/* $Log: sys_tmp.c,v $
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/tmp_ctl.h"

#include "../balance/engine.h"

#include "../machine/gate.h"
#include "../machine/intctl.h"

unsigned nonline = 1;	/* first online processor doesn't do tmp_ctl() */

/*
 * affinity()
 *	system call to allow process to bind itself to a processor (engine). 
 */

affinity()
{
	struct a {
		int	engno;
	} *uap = (struct a *)u.u_ap;
	register int engnum;
	register int oldeng;

	engnum = uap->engno;

	/*
	 * Just query current affinity.
	 */
	if (engnum == AFF_QUERY) {
		u.u_r.r_val1 = u.u_procp->p_affinity;
		return;
	}

	if (!suser()) {
		u.u_error = EPERM;
		return;
	}
	if ((engnum < 0 && engnum != AFF_NONE) || (engnum >= (int) Nengine)) {
		u.u_error = ENXIO;
		return;
	}
	if (engnum == AFF_NONE)
		engnum = ANYENG;

	/*
	 * Bind process to engine engnum.
	 */
	oldeng = runme(engnum);
	if (oldeng == E_UNAVAIL) {
		u.u_error = ENODEV;
		return;
	}
	if (oldeng == ANYENG)
		u.u_r.r_val1 = AFF_NONE;
	else
		u.u_r.r_val1 = oldeng;
}


/*
 * affinitypid()
 *	bind another quiescent process to an engine.
 */

affinitypid(pid, engno)
register int engno;
{
	register struct proc *p;
	register int oldeng;

	if (!suser()) {
		u.u_error = EPERM;
		return;
	}

	if (engno < 0 && engno != AFF_NONE && engno != AFF_QUERY ||
	    engno >= (int)Nengine) {
		u.u_error = ENXIO;
		return;
	}

	p = pfind(pid);
	if (p == NULL) {
		u.u_error = ESRCH;
		return;
	}

	oldeng = p->p_affinity;
	if (engno == AFF_QUERY) {
		/*
		 * Just query current affinity.
		 */
		u.u_r.r_val1 = oldeng;
		goto out;
	}

	if (oldeng == engno)
		/*
		 * Not changing affinity, nop.
		 */
		goto ok;

	if (p->p_stat != SSLEEP && p->p_stat != SSTOP) {
		/*
		 * Process is active, forget it.
		 */
		u.u_error = EBUSY;
		goto out;
	}

	if (engno == AFF_NONE)
		engno = ANYENG;

	/*
	 * Lock down engine table and bind process.
	 */
	(void)p_lock(&engtbl_lck, SPLHI);
	if (engno != ANYENG && (engine[engno].e_flags & E_NOWAY)) {
		u.u_error = ENODEV;
		v_lock(&engtbl_lck, SPLHI);
		goto out;
	}

	if (oldeng != ANYENG)
		engine[oldeng].e_count--;

	if (engno != ANYENG)
		engine[engno].e_count++;

	v_lock(&engtbl_lck, SPLHI);

	p->p_affinity = engno;
ok:
	if (oldeng == ANYENG)
		u.u_r.r_val1 = AFF_NONE;
	else
		u.u_r.r_val1 = oldeng;
out:
	v_lock(&p->p_state, SPL0);
	return;
}

sema_t	tmp_onoff;	/* sema for coordination of online(s) and offline(s) */

/*
 * tmp_ctl()
 *	Processor online/offline functions.
 *
 * Start (online) engines, stop (offline) engines, 
 * query online/offline status, and query number of engines configured.
 */

tmp_ctl()
{
	register struct a {
		int	command;
		u_int	engno;
		int	arg;
	} *uap = (struct a *)u.u_ap;
	register struct engine *eng;
	register int ret;
	spl_t  s_ipl;

	if (uap->command == TMP_NENG) {
		u.u_r.r_val1 = Nengine;
		return;
	}
	/*
	 * Validate engine number.
	 */
	if (uap->engno >= Nengine) {
		u.u_error = ENXIO;
		return;
	}
	eng = &engine[uap->engno];

	switch(uap->command) {

	case TMP_QUERY:
		u.u_r.r_val1 = eng->e_flags & E_OFFLINE;
		break;

	case TMP_OFFLINE:
		if (!suser()) {
			u.u_error = EPERM;
			break;
		}
		/*
		 * coordinate with concurrent online or offline.
		 */
		p_sema(&tmp_onoff, PZERO);
		/*
		 * If engine already offline.
		 */
		if (eng->e_flags & E_OFFLINE) {
			u.u_error = EINVAL;
			v_sema(&tmp_onoff);
			break;
		}
		/*
		 * If engine is bad.
		 */
		if (eng->e_flags & E_BAD) {
			u.u_error = ENODEV;
			v_sema(&tmp_onoff);
			break;
		}
		/*
		 * If this is the only processor online, or
		 * if the engine has a driver bound, or
		 * if there are processes bound to the engine,
		 * then return the EBUSY error code.
		 */
		if ((nonline == 1) || (eng->e_flags & E_DRIVER)) {
			u.u_error = EBUSY;
			v_sema(&tmp_onoff);
			break;
		}
		s_ipl = p_lock(&engtbl_lck, SPLHI);
		if (eng->e_count != 0) {
			v_lock(&engtbl_lck, s_ipl);
			u.u_error = EBUSY;
			v_sema(&tmp_onoff);
			break;
		}
#ifdef	FPA
		/*
		 * If there are FPA processes and the target processor is
		 * the last online processor with an FPA, no good.
		 */
		if ((eng->e_flags & E_FPA) && !offline_fpa(eng)) {
			v_lock(&engtbl_lck, s_ipl);
			u.u_error = EBUSY;
			v_sema(&tmp_onoff);
			break;
		}
#endif	FPA
		/*
		 * Shutdown OK, set shutdown request bit while
		 * holding engtbl_lck to coordinate with runme().
		 * Setting of shutdown request bit and nudge must
		 * be atomic to avoid race with [q]swtch.
		 */
		VOID_P_GATE(G_RUNQ);
		eng->e_flags |= E_SHUTDOWN;
		v_lock(&engtbl_lck, SPLHI);

		/*
		 * Nudge engine and wait for engine to see shutdown request.
		 */
		nudge(PUSER, eng);
		V_GATE(G_RUNQ, s_ipl);
#ifdef	i386
		splx(s_ipl);			/* V_GATE() doesn't drop spl */
#endif	i386
		p_sema(&eng_wait, PZERO);
		/*
		 * NOW halt the engine.
		 */
		halt_engine(eng);
		eng->e_flags |=  E_OFFLINE;
		eng->e_flags &= ~E_SHUTDOWN;
		nonline--;
		v_sema(&tmp_onoff);
		break;

	case TMP_ONLINE:
		if (!suser()) {
			u.u_error = EPERM;
			break;
		}
		p_sema(&tmp_onoff, PZERO);
		/*
		 * check if engine already online or
		 * if engine is bad (failed diagnostics).
		 */
		if ((eng->e_flags & E_OFFLINE) != E_OFFLINE) {
			u.u_error = EINVAL;
			v_sema(&tmp_onoff);
			break;
		}
		if (eng->e_flags & E_BAD) {
			u.u_error = ENODEV;
			v_sema(&tmp_onoff);
			break;
		}
		/*
		 * Start your engines. When initialized
		 * the started engine will V the eng_wait semaphore.
		 */
		start_engine(eng);

		/*
		 * Sleep until engine started.
		 */
		p_sema(&eng_wait, PZERO);
		nonline++;
		v_sema(&tmp_onoff);
		break;

	case TMP_GETFLAGS:
		(void)p_sema(&tmp_onoff, PZERO);
		/*
		 * Get the flags associated with a particular
		 * engine.
		 */
		ret = 0;
		if (!(eng->e_flags & E_OFFLINE))
			ret |= TMP_FLAGS_ONLINE;
		if (eng->e_flags & E_BAD)
			ret |= TMP_FLAGS_BAD;
#ifdef  FPA
		if (eng->e_flags & E_FPA)
			ret |= TMP_FLAGS_FPA;
#endif 	FPA
                u.u_r.r_val1 = ret;
		v_sema(&tmp_onoff);
		break;

	case TMP_TYPE:
#ifdef i386
                u.u_r.r_val1 = (eng->e_flags & E_SGS2) ? TMP_TYPE_I486 :
					TMP_TYPE_I386;
#endif
#ifdef ns32000
                u.u_r.r_val1 = TMP_TYPE_NS32000;
#endif
		break;
	case TMP_SLIC:
                u.u_r.r_val1 = eng->e_slicaddr;
		break;
	case TMP_RATE:
                u.u_r.r_val1 = eng->e_cpu_speed;
		break;
#ifdef i386
	case TMP_WPT:
		{
			struct kwpt kwpt;
			if (!(u.u_error = copyin((char *)uap->arg, 
						(char *)&kwpt, sizeof kwpt))) {
				u.u_error = kwatchpt(kwpt.kwpt_reg, 
						     kwpt.kwpt_val, 
						     kwpt.kwpt_act);
			}
		}
		break;
#endif
	default:
		u.u_error = EINVAL;
		break;
	}
}

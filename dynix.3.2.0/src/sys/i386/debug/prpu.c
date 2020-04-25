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
static	char	rcsid[] = "$Header: prpu.c 2.7 87/03/27 $";
#endif

/*
 * prpu.c
 *	Print the offsets of all proc and user fields.
 */

/* $Log:	prpu.c,v $
 */

#define	GENASSYM		/* so headers can drop decl's in X-environ */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/cmap.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/mbuf.h"

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"

#define	PF(f)	printf("%03X	f\n", &p->f)
#define	UF(f)	printf("%03X	f\n", &up->f)
#define	SZ(st)	printf("%03X	sizeof(struct st)\n", sizeof(struct st));

main()
{
	register struct proc *p = (struct proc *)0;
	register struct user *up = (struct user *)0;

	PF(p_link);
	PF(p_rlink);
	PF(p_affinity);
	PF(p_engno);
	PF(p_wchan);
	PF(p_yawc);
	PF(p_noswap);
	PF(p_vfork);
	PF(p_vflink);
	PF(p_state);
	PF(p_usrpri);
	PF(p_pri);
	PF(p_cpu);
	PF(p_stat);
	PF(p_time);
	PF(p_slptime);
	PF(p_flag);
	PF(p_pctcpu);
	PF(p_cpticks);
	PF(p_ppid);
	PF(p_mptpid);
	PF(p_pgrp);
	PF(p_pptr);
	PF(p_nice);
	PF(p_fpa);
	PF(p_cursig);
	PF(p_mptstop);
	PF(p_sig);
	PF(p_sigmask);
	PF(p_sigignore);
	PF(p_sigcatch);
	PF(p_uid);
	PF(p_suid);
	PF(p_pid);
	PF(p_ndx);
	PF(p_idhash);
	PF(p_swpq);
	PF(p_cptr);
	PF(p_sptr);
	PF(p_zombie);
	PF(p_ru);
	PF(p_xstat);
	PF(p_szpt);
	PF(p_dsize);
	PF(p_ssize);
	PF(p_rssize);
	PF(p_maxrss);
	PF(p_swaddr);
	PF(p_rshand);
	PF(p_rscurr);
	PF(p_pagewait);
	PF(p_spwait);
	PF(p_ptb1);
	PF(p_uarea);
	PF(p_ul2pt);
	PF(p_pttop);
	PF(p_realtimer);
	PF(p_mptc);
	PF(p_mpts);
	SZ(proc);

	printf("\n");

	UF(u_sp);
	UF(u_procp);
	UF(u_ar0);
	UF(u_pffcount);
	UF(u_pffvtime);
	UF(u_arg[0]);
	UF(u_fltaddr);
	UF(u_qsave);
	UF(u_error);
	UF(u_r);
	UF(u_eosys);
	UF(u_swtrap);
	UF(u_cred);
	UF(u_tsize);
	UF(u_dsize);
	UF(u_ssize);
	UF(u_dmap);
	UF(u_smap);
	UF(u_cdmap);
	UF(u_csmap);
	UF(u_ssave);
	UF(u_odsize);
	UF(u_ossize);
	UF(u_signal[0]);
	UF(u_sigtramp);
	UF(u_sigmask[0]);
	UF(u_sigonstack);
	UF(u_oldmask);
	UF(u_code);
	UF(u_segvcode);
	UF(u_sigstack);
	UF(u_ofile);
	UF(u_nofile);
	UF(u_lastfile);
	UF(u_lofile[0]);
	UF(u_ofile_ext);
	UF(u_cdir);
	UF(u_rdir);
	UF(u_ttyp);
	UF(u_ttyd);
	UF(u_cmask);
	UF(u_rlimit[0]);
	UF(u_fpusave);
#ifdef	FPA
	UF(u_fpasave);
#endif	FPA
	UF(u_mmap[0]);
	UF(u_mmapmax);
	UF(u_pmapcnt);
	UF(u_mmapdel);
	UF(u_sigpass);
	UF(u_mptdbgr);
	UF(u_universe);
	UF(u_tuniverse);
	UF(u_count);
	UF(u_offset);
	UF(u_fmode);
	UF(u_syst);
	UF(u_timer[0]);
	UF(u_start);
	UF(u_acflag);
	UF(u_scgacct);
	UF(u_ru);
	UF(u_cru);
	UF(u_prof);
	UF(u_comm[0]);
	SZ(user);
}

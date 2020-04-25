/* $Copyright:	$
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
static	char	rcsid[] = "$Header: init_main.c 2.29 91/02/12 $";
#endif

/*
 * init_main.c
 *	Final initializations, init/start system processes.
 */

/* $Log:	init_main.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/vfs.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/clist.h"
#include "../h/vnode.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"

/*
 * main()
 *	Initialization code.  Unix "main" procedure.
 *
 * Called from cold start routine after basic initializations
 * done (eg, sysinit()).  Running with stack on mapped U-area
 * of proc[0].
 *
 * Functions:
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 2 to page out
 *	     - process 1 execute bootstrap
 */

main(puarea)
	unsigned puarea;	/* phys (==virt) addr of proc[0] U-area */
{
	register int i;
	register struct proc *p;
	int s;
	extern	int	cmask;

	rqinit();
	mutexinit();				/* misc locks and semas */

	/*
	 * Set up system process 0 (swapper).  All fields are init'd to
	 * zero in sysinit().
	 */

	p = &proc[0];
	procmax = &proc[1];			/* init high-water */
	p->p_uarea = (struct user *)puarea;
	p->p_stat = SRUN;
	p->p_affinity = ANYENG;
	p->p_flag |= SLOAD|SSYS;
	p->p_nice = NZERO;
#ifdef	ns32000
	p->p_upte = &Sysmap[btop(p->p_uarea)];
	setredzone(p->p_upte, (caddr_t)&u);
#endif	ns32000

	u.u_procp = p;
	u.u_cmask = cmask;			/* default cmask */

	l.multprog = -1;			/* don't count proc[2] */

	nullvattr_init();			/* init the "null" vattr */

	/*
	 * Initialize kernel heap memory allocator
	 */
	kmem_init();
	
	/*
	 * RPC Initialization.
	 */
	svc_mutexinit();
	clnt_init();

	/*
	 * NFS Initialization.
	 */
	nfs_init();

	/*
	 * Get vnodes for swapdev and argdev.
	 */

	devnode_init();				/* init devnodes */
	swapdev_vp = devtovp(swapdev);

	/*
	 * Setup credentials.
	 */

	ucred_init();
	u.u_cred = crget();
	bzero((caddr_t)u.u_cred, sizeof(*u.u_cred));
	u.u_cred->cr_ref = 1;
	init_lock(&u.u_cred->cr_lock, G_CRED);

	for (i = 1; i < NGROUPS; i++)
		u.u_groups[i] = NOGROUP;

	for (i = 0; i < sizeof(u.u_rlimit)/sizeof(u.u_rlimit[0]); i++)
		u.u_rlimit[i].rlim_cur = u.u_rlimit[i].rlim_max = 
		    RLIM_INFINITY;
	if ((unsigned)defssiz > ctob(MAXDSIZ))
		defssiz = ctob(MAXDSIZ);
	u.u_rlimit[RLIMIT_STACK].rlim_cur = defssiz;
	u.u_rlimit[RLIMIT_STACK].rlim_max = ctob(MAXDSIZ);
	u.u_rlimit[RLIMIT_DATA].rlim_max =
	    u.u_rlimit[RLIMIT_DATA].rlim_cur = ctob(MAXDSIZ);
	p->p_maxrss = RLIM_INFINITY/CLBYTES;

#ifdef QUOTA
	qtinit();			/* init quota structures */
#endif

	/*
	 * Initialize tables and protocols.
	 */

	mbinit();			/* init/allocate mbuf's */
	cinit();			/* init/allocate c-list */

	/*
	 * initialize routing tables (currently routing is INET specific)
	 */

	rttabinit();
	loattach();			/* XXX */

	/*
	 * Block reception of incoming packets
	 * until protocols have been initialized.
	 */

	domaininit();

	s = splimp();
	ifinit();
	splx(s);

	procinit();				/* init proc-list locks/semas */
	ihinit();				/* inode hash/etc inits */
	bhinit();				/* init buffer hash queues */
	binit();				/* init buffer cache headers */
	finit();				/* file-table inits */
	mfinit();				/* mapped file table inits */
	xinit();				/* text inits */
	ptrcinit();				/* init ptrace semas */
	dnlc_init();				/* init dir lookup cache */
	flckinit();				/* initialize record locks */
	msginit();				/* message init */
	seminit();				/* semaphore init */

	swapconf();
	bswinit();				/* init swap buffers */

	/*
	 * mount the root, gets rootdir
	 * No need to obtain vfs_mutex since we're the only
	 * processor at this point and no other processes exist.
	 */

	vfs_mountroot();


	boottime = time;
	wdtinit();			/* initialize the watchdog timer */

	/*
	 * Kick off timeout driven events by calling first time.
	 */

	memerr();			/* polling for memory errors */
	schedcpu();
	timeslice();

	u.u_dmap = zdmap;
	u.u_smap = zdmap;

	/*
	 * Make init process.
	 */

	mpid = 0;
	proc[1].p_stat = 0;
	proc[0].p_szpt = SZPT(&proc[0]);	/* so proc[1] gets PT's */
	proc[0].p_rscurr = vmtune.vt_minRS;	/* and a minimal Rset array */
	if (newproc(0)) {

		/*
		 * We are proc[1] at this point, and have U-area and
		 * page-tables.  Now expand and copy out the "init-code"
		 * which will exec /etc/init.
		 */

		(void) vsalloc(&u.u_dmap, 0, u.u_dsize);
		(void) vsalloc(&u.u_smap, 0, u.u_ssize);
		(void) expand((int)clrnd(btoc(szicode) + LOWPAGES),
				(int)clrnd(btoc(1)), 0, PG_ZFOD);
		(void) copyout(icode, (caddr_t)(LOWPAGES*NBPG), (unsigned)szicode);

		/*
		 * Setup to verify running correct version of system.
		 */

		check_sys(1);

		/*
		 * Return goes to location LOWPAGES*NBPG of user init code
		 * just copied out.
		 */

		return (0);				/* value for lint */
	}

	/*
	 * Make page-out daemon (process 2).
	 *
	 * Ok to create this after creating proc[1] since shouldn't
	 * dispatch (eg, allow [1] to execute) until after pageout()
	 * blocks.  Would be ok anyhow, since would only postpone
	 * start of pageout IO.
	 *
	 * p_dsize of proc[2] is to allow mapping of pages during
	 * pageouts (avoiding races with other concurrent uses of
	 * page-tables).
	 */

	proc[0].p_rscurr = 0;
	proc[0].p_szpt = clrnd(SZL2PT(nswbuf*CLSIZE*KLMAX, 0) + UL1PT_PAGES);
	if (newproc(0)) {
		proc[2].p_flag |= SLOAD|SSYS;
		proc[2].p_noswap++;			/* insure no swapping */
		proc[2].p_dsize = u.u_dsize = nswbuf*CLSIZE*KLMAX; 
		pageout();
		/*NOTREACHED*/
	}

	/*
	 * Enter swap-scheduling loop.
	 */

	proc[0].p_szpt = SZPT(&proc[0]);	/* reflect reality */
	sched();
	/*NOTREACHED*/
}

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
cinit()
{
	register int ccp;
	register struct cblock *cp;
#ifdef PERFSTAT
	extern int cfreelow;
#endif PERFSTAT

	ccp = (int)cfree;
	ccp = (ccp+CROUND) & ~CROUND;
	for(cp=(struct cblock *)ccp; cp < &cfree[nclist-1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}

#ifdef PERFSTAT
	cfreelow = cfreecount;
#endif PERFSTAT
}

/*
 * procinit()
 *	Init semaphores and locks in each proc table entry.
 */

procinit()
{
	register struct proc *p;
	register int	lkgate;

	/*
	 * Init proc-list locker.  TMP only sema.
	 */

	init_lock(&proc_list, G_PLIST);

	/*
	 * Init the lock's/sema's for each proc entry.
	 * Gates G_PSTMIN..G_PSTMAX are distributed among the p_state locks.
	 */

	for(p = proc, lkgate = 0; p < procNPROC; p++, lkgate++) {
		init_sema(&p->p_pagewait, 0, 0, G_PPGWAIT);
		init_sema(&p->p_zombie, 0, 0, G_PZOMBIE);
		init_sema(&p->p_vfork, 0, 0, G_PVFORK);
		init_lock(&p->p_state, (gate_t)(G_PSTMIN + lkgate % (G_PSTMAX-G_PSTMIN+1)));
	}
#ifdef	lint
	lint_ref_int(lkgate);
#endif	lint
}

/*
 * mutexinit()
 *	Init semaphores not yet initialized.
 */

mutexinit()
{
	extern sema_t pause;
	extern sema_t acct_sema;
	extern sema_t tmp_onoff;
	extern sema_t dir_rename_mutex;
	extern sema_t sync_sema;

#if	defined(MFG)
	extern lock_t core_lock;
	init_lock(&core_lock, G_COREDUMP);	/* insure unique names */
#endif	MFG

	init_sema(&acct_sema, 1, 0, G_ACCT);	/* sema for accounting */
 	init_sema(&dir_rename_mutex, 1, 0, G_INOMAX); /* see direnter() */
	init_sema(&lbolt, 0, 0, G_LBOLT);	/* sema for lighning bolt */
	init_sema(&pause, 0, 0, G_PAUSE);	/* sema for sigpause sleep */
	init_lock(&select_lck, G_SELECT);
 	init_sema(&selwait, 0, 0, G_SELECT);	/* sema for selwait sleep */
 	init_sema(&tmp_onoff, 1, 0, G_ENGINE);	/* sema for on/off-line */
	init_sema(&vfs_mutex, 1, 0, G_FS);	/* For mount,umount */
	init_sema(&unmount_mutex, 1, 0, G_FS);	/* Serialize umounts */
	init_sema(&sync_sema, 1, 0, G_FS);	/* sync mutex */
}

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

/*
 * $Header: proc.h 2.10 90/11/08 $
 *
 * proc.h
 *	Memory resident process structure.
 *
 * All references to "quota" are removed.
 * Many flags used to block swapper are removed, using p_noswap instead.
 */

/* $Log:	proc.h,v $
 */

/*
 * One structure allocated per active process. It contains all data needed
 * about the process while the process may be swapped out.
 *
 * Other per process data (user.h) is swapped with the process.
 */

struct	proc	{
	/*
	 * Links used by run-Q's and sema Q's.
	 */
	struct	proc	*p_link;	/* linked list of running processes */
	struct	proc	*p_rlink;
	/*
	 * For processor/process affinity.
	 */
	short		p_affinity;	/* Engine proc must run on */
	short		p_engno;	/* Engine proc running on */
	/*
	 * Semaphore related fields.
	 */
	sema_t		*p_wchan;	/* event process is awaiting */
	caddr_t		p_yawc;		/* yet-another-wchan (see sleep()) */
	int		p_noswap;	/* count of reasons not to swap */
	/*
	 * Vfork related fields.
	 */
	sema_t		p_vfork;	/* vfork synchronization */
	struct	proc	*p_vflink;	/* parent->child chain */
	/*
	 * State variables, gated via p_state.
	 */
	lock_t		p_state;	/* lock for state info */
	char		p_usrpri;	/* user-prio based on p_cpu & p_nice */
	char		p_pri;		/* priority, negative is high */
	char		p_cpu;		/* cpu usage for scheduling */
	char		p_stat;		/* current process state */
	char		p_time;		/* resident time for scheduling */
	char		p_slptime;	/* time since last block */
	int		p_flag;		/* various flags; see below */
	long		p_pctcpu;	/* %cpu for this proc during p_time */
	short		p_cpticks;	/* ticks of cpu time */
	short		p_ppid;		/* process id of parent */
	short		p_mptpid;	/* pid of MP debugger */
	short		p_pgrp;		/* name of process group leader */
	struct	proc	*p_pptr;	/* -> process structure of parent */
	char		p_nice;		/* nice for cpu usage */
	char		p_fpa;		/* process using FPA? */
#define FPA_NONE        0               /* no usage of the FPA entirely */
#define FPA_HW          1               /* hardware usage of the FPA */
#define FPA_SW          2               /* software emulation of the FPA */
#define FPA_FRCSW       3               /* force the process to use the */
					/* emulation (testing only) */

	/*
	 * Signal fields; also gated by p_state.
	 */
	char		p_cursig;
	char		p_mptstop;	/* reason for MP debugger stop */
	int		p_sig;		/* signals pending to this process */
	int		p_sigmask;	/* current signal mask */
	int		p_sigignore;	/* signals being ignored */
	int		p_sigcatch;	/* signals being caught by user */
	/*
	 * Quasi-static fields gated by proc_list.
	 */
	short		p_uid;		/* UID; mostly RO, but let fork count */
	short		p_suid;		/* saved effective UID */
	short		p_pid;		/* unique process id */
	short		p_ndx;		/* proc idx for memall (due to vfork) */
	short		p_idhash;	/* based on p_pid for kill+exit+... */
	/*
	 * Queue for swapout, based on proc index; zero terminates list 
	 * (swapper doesn't swap self).
	 */
	short		p_swpq;		/* queue for swapout */
	/*
	 * Child/sibling fields; gated by parent.
	 */
	struct	proc	*p_cptr;	/* pointer to head of child list */
	struct	proc	*p_sptr;	/* pointer to sibling processes */
	sema_t		p_zombie;	/* for waiting on dead children */
	/*
	 * Fields used only by child, then parent after wait() (ie, no gating).
	 */
	struct	rusage	*p_ru;		/* mbuf holding exit information */
	u_short		p_xstat;	/* Exit status for wait */
	/*
	 * Paging related fields.  Gated implicitly.
	 */
	short		p_szpt;		/* # HW pages to hold page-table */
	size_t		p_dsize;	/* # HW pages of text+data */
	size_t		p_ssize;	/* # HW pages of stack */
	size_t 		p_rssize; 	/* # user pages in Rset */
	size_t		p_maxrss;	/* copy of u.u_limit[MAXRSS] */
	swblk_t		p_swaddr;	/* disk addr of u area when swapped */
	/*
	 * Implementation specific paging fields.
	 */
	size_t		p_rshand;	/* RS hand for replacement */
	long		p_rscurr;	/* current RS size */
	sema_t		p_pagewait;	/* waiting for in-transit page */
	struct	proc	*p_spwait;	/* waiters list for in-transit page */
	unsigned	p_ptb1;		/* phys address of UL1PT for dispatch */
	struct	user	*p_uarea;	/* u-area kernel virtual address */
#ifdef	ns32000
	struct	pte	*p_upte;	/* point at U-area pte's for dispatch */
#endif	ns32000
	struct	pte	*p_ul2pt;	/* virtual address of UL2PT */
	struct	pte	*p_pttop;	/* top of UL2PT, for efficiency */
	/*
	 * timer related.
	 */
	struct itimerval p_realtimer;
	/*
	 * Multi-process debugger process list.  Gated by proclist.
	 * Maintained as seperate list from p_cptr,p_sptr since
	 * intermediate process can exit.
	 */
	u_short		p_mptc;		/* multi-process trace child */
	u_short		p_mpts;		/* multi-process trace sibling */
};

#define	PIDHSZ		64
#define	PIDHASH(pid)	((pid) & (PIDHSZ - 1))

#ifdef KERNEL
struct	proc	*pfind();
struct	proc	*lpfind();
extern	short	pidhash[PIDHSZ];
extern	struct	proc *proc, *procNPROC;	/* the proc table itself */
extern	struct	proc *procmax;		/* high-water pointer */
extern	lock_t	proc_list;		/* proc-list locker */
extern	int	nproc;

/*
 * QUEUEDSIG(p) is true if process has a queued signal it should look at.
 */

#define	QUEUEDSIG(p)	((p)->p_sig && ((p)->p_flag&SDBG || \
			((p)->p_sig & ~((p)->p_sigignore | (p)->p_sigmask))))
#endif

/* affinity code */
#define	ANYENG -2

/* stat codes */
#define	SSLEEP	1		/* awaiting an event */
#define	SWAIT	2		/* (abandoned state) */
#define	SRUN	3		/* runnable (on runQ's) */
#define	SIDL	4		/* intermediate state in process creation */
#define	SZOMB	5		/* intermediate state in process termination */
#define	SSTOP	6		/* process being traced */
#define	SONPROC	7		/* process running on a processor */

/*
 * Flag codes.
 */

#define	SLOAD	0x0000001	/* in core */
#define	SSYS	0x0000002	/* swapper or pager (system) process */
#define	STRC	0x0000010	/* process is being traced */
#define	SWTED	0x0000020	/* another tracing flag */
#define	SOMASK	0x0000080	/* restore old mask after taking signal */
#define	SVFORK	0x0000100	/* process resulted from vfork() */
#define	SNOVM	0x0000200	/* no vm, parent in a vfork() */
#define	STIMO	0x0000400	/* timing out during sleep */
#define	SOUSIG	0x0000800	/* using old signal mechanism */
#define	SSEL	0x0001000	/* selecting; wakeup/waiting danger */
#define	SIGWOKE	0x0002000	/* signal woke process up, not v_sema() */
#define	SWPSYNC	0x0004000	/* synch with swapper during swapout */
#define	SFSWAP	0x0008000	/* force swap self, swapper couldn't */
#define	SNOSWAP	0x0010000	/* process can't be swapped (set via syscall) */
#define	SNOAGE	0x0020000	/* don't age process priority */

#define	SMPDBGR	0x0040000	/* process is a multi-process debugger */
#define	SMPTRC	0x0080000	/* process debugged by a MP debugger */
#define	SMPSTOP	0x0100000	/* process is stopped for MP debugger */
#define	SMPWTED	0x0200000	/* MP debugger has seen stopped process */
#define	STRCSTP	0x0400000	/* process is stopped for old flavor debugger */
#define	SNOPFF	0x0800000	/* process shouldn't auto-adjust resident-set */

/*
 * SMPTRC is only turned on/off while holding both proc_list and process
 * state.  Thus holding either lock guarantees consistency of SMPTRC.
 */

/*
 * SFORK defines flags unconditionally inherited across fork.
 * SDBG defines flags that imply process is being debugged (traced). 
 */

#define	SFORK	(SLOAD|SOUSIG|SNOSWAP|SNOAGE|SNOPFF)
#define	SDBG	(STRC|SMPTRC)

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
 * $Header: ptrace.h 2.7 90/06/12 $
 *
 * ptrace.h
 *	Definitions for ptrace(2) usage, including multi-process
 *	debugging hooks (Sequent specific extensions).
 *
 * i386 version.
 */

/* $Log:	ptrace.h,v $
 */

/*
 * Mnemonics for sub-functions.
 * The XPT_ functions are Sequent extensions to standard UNIX ptrace().
 * A subset of these are useable without becomming a multi-process debugger
 * (XPT_MPDEBUGGER; see ptrace(2)).
 */

#define	PT_CHILD		0	/* caller is ptrace child */
#define	PT_RTEXT		1	/* read process text */
#define	PT_RDATA		2	/* read process data */
#define	PT_RUSER		3	/* read process struct user */
#define	PT_WTEXT		4	/* write process text */
#define	PT_WDATA		5	/* write process data */
#define	PT_WUSER		6	/* write (subset) process struct user */
#define	PT_CONTSIG		7	/* continue with signal */
#define	PT_KILL			8	/* process dies */
#define	PT_SSTEP		9	/* single-step with signal */

#define	XPT_MPDEBUGGER		10	/* caller is multi-process debugger */
#define	XPT_SETSIGMASK		11	/* set signal pass-thru mask */
#define	XPT_RREGS		12	/* read process registers */
#define	XPT_WREGS		13	/* write process registers */
#define	XPT_RPROC		14	/* read process struct proc */
#define	XPT_SIGNAL		15	/* signal process or all */
#define	XPT_STOPSTAT		16	/* get stopped status */
#define	XPT_DUPFD		17	/* dup fd to process */
#define	XPT_DEBUG		18	/* start debugging unrelated process */
#define	XPT_OPENT		19	/* return fd for text file */
#define	XPT_WATCHPT_SET		20	/* set a data watchpoint */
#define	XPT_WATCHPT_CLEAR	21	/* clear a data watchpoint */
#define	XPT_UNDEBUG		22	/* quit debugging a process */

#define	XPT_MAX_FUNCT		22	/* largest legal function number */

/*
 * Stop structure and #defines.  Filled by XPT_STOPSTAT.
 *
 * ps_reason is a signal number (1 - 32) or one of PTS_FORK, PTS_EXEC,
 * PTS_EXIT, PTS_WATCHPT_AFTER (i386 doesn't support PTS_WATCHPT_BEFORE).
 */

struct	pt_stop	{
	int	ps_pid;			/* pid of stopped process */
	int	ps_reason;		/* reason for the stop */
};

#define	PTS_FORK		33	/* new child of debugged process */
#define	PTS_EXEC		34	/* process successfully exec'd */
#define	PTS_EXIT		35	/* process about to die */
#define	PTS_WATCHPT_BEFORE	36	/* watchpoint before write */
#define	PTS_WATCHPT_AFTER	37	/* watchpoint after write */

/*
 * Structure to define a data breakpoint (watchpoint) in a
 * debuggee process.  This is pointed to by the "data" argument
 * of the XPT_WATCHPT_SET function.
 */

struct	pt_watchpt {
	caddr_t wp_addr;		/* address to set bpt at */
	int	wp_length;		/* extent of bpt */
	int	wp_flags;		/* read/write/execute */
};

/*
 * Values for wp_flags (or-able).
 */

#define PWP_READ		1	/* trap reads to location */
#define PWP_WRITE		2	/* trap writes to location */
#define PWP_XEQ			4	/* trap execution of location */

/*
 * Register set used by XPT_{R,W}REGS.  Structure is machine dependent.
 * Floating point registers read as zeroes if process not using
 * FPU instructions.
 *
 * Format of floating point part must match that in struct user.
 */

struct	pt_regset {
	/*
	 * General registers and processor status.
	 */
	int	pr_eax;
	int	pr_ebx;
	int	pr_ecx;
	int	pr_edx;
	int	pr_esi;
	int	pr_edi;
	int	pr_ebp;
	int	pr_esp;
	int	pr_eip;
	int	pr_flags;
	/*
	 * Floating point unit registers and status.
	 */
	struct	fpusave	pr_fpu;
#ifdef	FPA
	/*
	 * Floating point accelerator registers and status.
	 */
	struct	fpasave	pr_fpa;
#endif	FPA
};

#ifdef	KERNEL
/*
 * Machine independent register locators.
 */

#define	pr_genfirst	pr_eax
#define	pr_psw		pr_flags

/*
 * Various machine-independent constants for procxmt().
 *
 * Note that IPC_PSW_USRCLR dis-allows setting FLAGS_VM (thus can't use
 * this to enter virtual 8086 mode); kernel return to virtual 8086 mode
 * requires different stack frame, thus need special kernel entry to
 * turn on VM86 and handle the traps.
 */

#define	IPC_PC		EIP				/* stacked user PC */
#define	IPC_PSW		FLAGS				/* stacked user PSW */
#define	IPC_PSW_USRSET	FLAGS_IF			/* bits must be set */
#define	IPC_PSW_USRCLR	(FLAGS_VM|FLAGS_RF|FLAGS_IOPL|FLAGS_NT|FLAGS_RSVD)
#define	IPC_PSW_SSTEP	FLAGS_TF			/* single-step bit */

/*
 * Priority for tracing
 */

#define	IPCPRI	PZERO

/*
 * Tracing variables.
 *
 * Used to pass trace command from debugger to process being traced.
 * This data base cannot be shared and is locked per user.
 *
 * If this single resource is a problem, should consider ways to
 * have multiple of them.
 */

extern	struct	dbg_ipc	{
	sema_t		ip_lock;		/* for access to structure */
	sema_t		ip_synch;		/* to synch with "child" */
	int		ip_req;
	caddr_t		ip_addr;
	int		ip_data;
	struct	pt_regset  ip_regs;		/* for get/set register cmds */
	struct	pt_watchpt ip_watchpt;		/* for watchpoint cmds */
	int		ip_error;		/* returned error, if any */
} ipc;
#endif	KERNEL

/* 
 * Per-process watchpoint structure.
 */

#define	NUM_WATCHPT	4		/* number of HW supported watchpoints */

struct	watchpt	{
	u_int	wp_vaddr[NUM_WATCHPT];	/* virtual address of watchpoint */
	u_int	wp_control;		/* copy of control register */
};

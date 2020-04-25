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

/*
 * $Header: user.h 2.35 1992/02/13 00:20:52 $
 *
 * user.h
 *	Per process structure containing data that
 *	isn't needed in core when the process is swapped out.
 */

/* $Log: user.h,v $
 *
 *
 */

#ifdef KERNEL
#include "../h/dmap.h"
#include "../h/time.h"
#include "../h/resource.h"
#include "../h/mman.h"
#include "../h/ofile.h"
#include "../h/universe.h"
#include "../machine/fpu.h"
#include "../machine/ptrace.h"
#else
#include <sys/dmap.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/ofile.h>
#include <sys/universe.h>
#include <machine/fpu.h>
#include <machine/ptrace.h>
#endif

/*
 * Per-process swappable process-representation (U-area).
 */

#define	MAXCOMLEN	36			/* sizeof u_comm, <= MAXNAMLEN*/

struct	user	{
	caddr_t		u_sp;			/* saved sp (context switch) */
	struct	proc	*u_procp;		/* pointer to proc structure */
	int		*u_ar0;			/* address of users saved R0 */

	/*
	 * Page-fault frequency related fields.
	 */
	long		u_pffcount;		/* #faults since last adjust */
	long		u_pffvtime;		/* virt-time since last adjust*/

	/*
	 * Syscall parameters, results and catches.
	 */
	int		u_arg[6];		/* args to current syscall */
#define	u_ap		u_arg			/* pointer to arglist */
#ifdef	i386
	unsigned	u_fltaddr;		/* addr for kernel page fault */
#endif	i386
	label_t		u_qsave;		/* non-local gotos on signals */
	char		u_error;		/* return error code */
	union	{				/* syscall return values */
		struct	{
			int	R_val1;
			int	R_val2;
		} u_rv;
#define	r_val1	u_rv.R_val1
#define	r_val2	u_rv.R_val2
		off_t	r_off;
		time_t	r_time;
	} u_r;
	char		u_eosys;		/* end syscall special action */
#ifdef	ns32000
	u_char		u_swtrap;		/* SW traps (see below) */
#endif	ns32000
#ifdef	i386
	u_int		u_swtrap;		/* SW traps (see below) */
#endif	i386

	/*
	 * Processes and protection
	 */
	struct	ucred	*u_cred;		/* user credentials */
#define	u_uid	u_cred->cr_uid
#define	u_gid	u_cred->cr_gid
#define	u_groups u_cred->cr_groups
#define	u_ruid	u_cred->cr_ruid
#define	u_rgid	u_cred->cr_rgid

	/*
	 * Memory management
	 * u_tsize holds only the initial size of the text after exec.
	 * u_dsize holds the sum of text and data.  mmap() can affect u_dsize.
	 */
	size_t		u_tsize;		/* text size (clicks) */
	size_t		u_dsize;		/* text+data size (clicks) */
	size_t		u_ssize;		/* stack size (clicks) */
	struct	dmap	u_dmap;			/* disk map for data segment */
	struct	dmap	u_smap;			/* disk map for stack segment */
	struct	dmap	u_cdmap, u_csmap;	/* shadows u_dmap, u_smap, for
						   use of parent during fork */
	label_t		u_ssave;		/* label variable for swap */
	size_t		u_odsize, u_ossize;	/* for expansion swaps */

	/*
	 * Signal management
	 */
	int		(*u_signal[NSIG+1])();	/* disposition of signals */
	int		(*u_sigtramp)();	/* signal trampoline code */
	int		u_sigmask[NSIG+1];	/* signals to be blocked */
	int		u_sigonstack;		/* signals taken on sigstack */
	int		u_oldmask;		/* saved mask for sigpause */
	int		u_code;			/* ``code'' to trap */
	int		u_segvcode;		/* address for SIGSEGV hdlr */
	struct	sigstack u_sigstack;		/* sp & on stack state */
#define	u_sigsp		u_sigstack.ss_sp
#define	u_onstack	u_sigstack.ss_onstack

	/*
	 * Descriptor management
	 */
	struct	ofile_tab *u_ofile_tab;		/* open-file table */
	struct	file	*u_fpref;		/* syscall made ref to this */
	int		u_nofileXXX;		/* should go away */
	int		u_lastfileXXX;		/* should go away */
	struct	ofile	u_lofileXXX[NOFILE];	/* should go away */
	struct	vnode	*u_cdir;		/* current directory */
	struct	vnode	*u_rdir;		/* root dir of current process*/
	struct	tty	*u_ttyp;		/* controlling tty pointer */
	dev_t		u_ttyd;			/* controlling tty dev */
	short		u_cmask;		/* mask for file creation */

	/*
	 * Resource controls
	 */
	struct	rlimit	u_rlimit[RLIM_NLIMITS];

	/*
	 * Floating-point save area.
	 */
	struct	fpusave	u_fpusave;		/* float unit save area */
#ifdef	FPA
	struct	fpasave	u_fpasave;		/* float accel. save area */
#endif	FPA

	/*
	 * Mmap state information.
	 * All u_mmap[]'s in use are < u_mmapmax; cleared in exec, only grows.
	 * u_mmapdel is a boolean, tells if must copy back u_mmap[]
	 * array into parent after vfork child exit|exec's.
	 */
	struct	mmap	u_mmap[NUMMAP];		/* per mapped object */
	struct	mmap	*u_mmapmax;		/* valid mmaps < mmapmax */
	char		u_pmapcnt;		/* # MM_PHYS maps in use */
	char		u_mmapdel;		/* page delted? for vfork */

	/*
	 * Multi-process debugging fields.
	 */
	int		u_sigpass;		/* mask of pass-thru signals */
	struct	proc	*u_mptdbgr;		/* MP debugger in control */
#ifdef	i386
	struct	watchpt	u_watchpt;		/* watchpoint support */
#endif	i386

/* BEGIN TRASH */
	char		u_universe;		/* flag to indicate universe */
	char		u_tuniverse;		/* flag ==> sysV syscall */
	unsigned int	u_count;		/* bytes remaining for IO */
	off_t		u_offset;		/* offset in file for IO */
	int		u_fmode;		/* added for sV pipes */
/* END TRASH */

	/*
	 * Timing and statistics
	 */
	struct	timeval	u_syst;			/* proc time on kernel entry */
	struct	itimerval u_timer[3];
	struct	timeval	u_start;
	long		u_ioch;			/* chars transferred */
	short		u_acflag;
	char		u_scgacct;		/* account identifier */
	struct	rusage	u_ru;			/* stats for this proc */
	struct	rusage	u_cru;			/* sum of reaped child stats */
	struct	uprof	{			/* profile arguments */
		short	*pr_base;		/* buffer base */
		unsigned pr_size;		/* buffer size */
		unsigned pr_off;		/* pc offset */
		unsigned pr_scale;		/* pc scaling */
	} u_prof;

	/*
	 * Exec command save area.
	 */
#ifdef	ns32000
	char		u_comm[MAXCOMLEN + 1];	/* exec path last component */
#endif	ns32000
#ifdef	i386					/* No Chip Bugs! */
	char		u_comm[MAXCOMLEN + 1];	/* exec path last component */
#endif	i386

	struct fpaesave *u_fpaesave;            /* ptr to emulation save area */
	long            u_fpae_extra_status;
	unsigned short  u_flags;                /* Misc. flags */

	/*
	 * The following space is reserved for future use.  If the
	 * NEED arises to add another field to the user structure, we
	 * can use a portion of the u_pad space, thus maintaining binary
	 * compatibility.  Also, in the unlikely event that we no longer 
	 * need an existing entry, the excess space should be returned to 
	 * u_pad (to maintain the exact size of the uarea).
	 */
	unsigned char   u_pad[2];
	/*
	 * This must be the last structure element.
	 */
	int		u_stack[1];

};

/* u_eosys values */
#define	JUSTRETURN	0
#define	RESTARTSYS	1

/* u_error codes */
#include <errno.h>

/*
 * u_flags bit positions
 */
#ifdef COBUG
#define UF_FPSTEP       1       /* Are stepping a single FPU instr */
#define UF_OTBIT        2       /* Were in trace mode before */
#endif /* COBUG */
#ifdef	FPU_SIGNAL_BUG
#define UF_USED_FPU     0x0004  /* Process used fpu at least once in lifetime*/
#define UF_SAVED_FPU    0x0008  /* Used by sendsig()/sigcleanup() */
#define UF_SAVED_FPA    0x0010  /* Used by sendsig()/sigcleanup() */
#endif  /* FPU_SIGNAL_BUG */

/*
 * User credential structure.  Holds information for identity and access
 * checking.
 */

struct	ucred	{
	lock_t		cr_lock;		/* Mutex for ref count */
	short		cr_ref;			/* reference count */
	short		cr_uid;			/* effective user id */
	short		cr_gid;			/* effective group id */
	int		cr_groups[NGROUPS];	/* groups, filled w/NOGROUP */
	short		cr_ruid;		/* real user id */
	short		cr_rgid;		/* real group id */
};

#ifdef	KERNEL
extern	struct user u;

/*
 * Software traps queue work at interrupt level to be done in a process
 * before re-enterring user mode.  They handle various per-process bookkeepping
 * functions.  Most SW traps are turned on only in an interrupt from
 * user mode, thus guarantee the code will be executed before the process
 * returns from the kernel.  Callers of SWTON() should insure the test/set
 * of l.runrun can't be interrupted (typically invoked only at SPLHI).
 */

#define	SWT_PFF		0x01			/* PFF adjustment */
#define	SWT_PROF	0x02			/* PROFile tick */

#define	SWT_FPU_PGXBUG	0x80			/* FPU Instr Cross Page Bug */

#define	SWTON(mask) { \
	u.u_swtrap |= (mask); \
	if (l.runrun == 0) l.runrun = -1; \
}

/*
 * Credential manipulation procedures, public data-structures.
 */

struct	ucred	*crget();
struct	ucred	*crcopy();
struct	ucred	*crdup();
void		crhold();
void		crfree();

extern	struct	ucred	*ucred_free;		/* credentials free list */
extern	int		nucred;			/* # cred's allocated */


#endif	KERNEL

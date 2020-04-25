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
 * $Header: vmmeter.h 2.0 86/01/28 $
 *
 * Virtual memory related instrumentation
 */

/* $Log:	vmmeter.h,v $
 */

#define	CPUSTATES	4

#define	CP_USER		0
#define	CP_NICE		1
#define	CP_SYS		2
#define	CP_IDLE		3

struct vmmeter
{
	/*
	 * Misc. system statistics
	 */
#define	v_first	v_swtch
	unsigned v_swtch;	/* context switches */
	unsigned v_trap;	/* calls to trap */
	unsigned v_syscall;	/* calls to syscall() */
	unsigned v_intr;	/* device interrupts */
	/*
	 * Swap statistics
	 */
	unsigned v_swpin;	/* swapins */
	unsigned v_swpout;	/* swapouts */
	unsigned v_pswpin;	/* pages swapped in */
	unsigned v_pswpout;	/* pages swapped out */
	/*
	 * Paging  statistics
	 */
	unsigned v_pgin;	/* pageins */
	unsigned v_pgout;	/* pageouts */
	unsigned v_pgpgin;	/* pages paged in */
	unsigned v_pgpgout;	/* pages paged out */
	unsigned v_intrans;	/* intransit blocking page faults */
	unsigned v_pgrec;	/* total page reclaims */
	unsigned v_pgfrec;	/* page reclaims from free list */
	unsigned v_pgdrec;	/* dirty page reclaims */
	unsigned v_exfod;	/* pages filled on demand from executables */
	unsigned v_zfod;	/* pages zero filled on demand */
	unsigned v_nexfod;	/* number of exfod's created */
	unsigned v_nzfod;	/* number of zfod's created */
	unsigned v_realloc;	/* ptes realloc'd before reclaimed */
	unsigned v_redofod;	/* ptes re-built to FOD (realloc'd pre-paged) */
	unsigned v_faults;	/* total faults taken */
	unsigned v_dfree;	/* pages freed by pageout daemon */
	/*
	 * Cpu time statistics
	 */
	unsigned v_time[CPUSTATES];
	/*
	 * Tty statistics
	 */
	unsigned v_ttyin;	/* no. of tty chars input */
	unsigned v_ttyout;	/* no. of tty chars output */
	/*
	 * Fork/Vfork statistics
	 */
	unsigned v_cntfork;
	unsigned v_cntvfork;
	unsigned v_sizfork;
	unsigned v_sizvfork;
	/*
	 * Exec statistics
	 */
	unsigned v_cntexec;	/* number of execs */
	/*
	 * Buf Cache statistics
	 */
	unsigned v_lreads;	/* logical read count ie. bread?() */
	unsigned v_lwrites;	/* logical write count ie. bwrite?() */
	unsigned v_racount;	/* read ahead call count */
	unsigned v_rcount;	/* number of reads actual to disk */
	unsigned v_wcount;	/* number of writes that call driver strat */
	unsigned v_rainbc;	/* count of r.a. blks was already in bcache */
	unsigned v_rawasted;	/* count of unused r.a. blks bcache dropped */
	unsigned v_bhitra;	/* Hit and block was in because of r.a. */
	unsigned v_bhitru;	/* Hit and block was in because of reuse */
	/*
	 * Physio stats
	 */
	unsigned v_phyrc;	/* Number of physio (read) driver strat calls */
	unsigned v_phywc;	/* Number of physio (write) drver strat calls */
	unsigned v_phyr;	/* Number of blocks (DEV_BSIZE) read */
	unsigned v_phyw;	/* Number of blocks (DEV_BSIZE) write */

#define	v_last v_phyw
};

/*
 * Systemwide totals computed every five seconds.
 */
struct vmtotal
{
	long	t_rq;		/* length of the run queue */
	long	t_dw;		/* jobs in ``disk wait'' (neg priority) */
	long	t_pw;		/* jobs in page wait */
	long	t_sl;		/* jobs sleeping in core */
	long	t_sw;		/* swapped out runnable/short block jobs */
	long	t_vm;		/* total virtual memory */
	long	t_avm;		/* active virtual memory */
	long	t_rm;		/* total real memory in use */
	long	t_arm;		/* active real memory */
	long	t_free;		/* free memory pages */
};

#ifdef	KERNEL
extern	struct	vmtotal total;
#endif	KERNEL

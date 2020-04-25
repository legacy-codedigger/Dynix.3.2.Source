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
static	char	rcsid[] = "$Header: param.c 2.53 1991/05/29 23:04:52 $";
#endif

/*
 * param.c
 *	Configuration parameters file.  This is compiled as part of the
 *	kernel configuration process.
 *
 * NOTE TO DEVELOPERS: These parameters are documented in Appendix D
 * of the article "Building DYNIX Systems with Config."  If you add,
 * delete, or modify a parameter, be sure to update the corresponding
 * text in src/doc/config/d.t.
 */

/* $Log: param.c,v $
 *
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/socket.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/file.h"
#include "../h/callout.h"
#include "../h/clist.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/kernel.h"
#include "../h/dk.h"
#include "../h/map.h"
#include "../h/flock.h"

#include "../balance/clock.h"

/*
 * System parameter formulae.
 *
 * This file is copied into each directory where we compile
 * the kernel; it should be modified there to suit local taste
 * if necessary.
 *
 * Compiled with -DTIMEZONE=x -DDST=x -DMAXUSERS=xx
 */

#define	K	1024

/*
 * Table sizing heuristics are based on "maxusers" parameter and a modifier
 * to more closely match a typical "timeshare", "parallel/research",
 * or "commercial" system.
 *
 * All interesting parameters can be individually modified via "options"
 * lines in the configuration file to compensate for the heuristics when
 * they aren't optimal for a given system.
 */

#if	!defined(COMMERCIAL) && !defined(PARALLEL) && !defined(TIMESHARE)
	ERROR -- Specify one of TIMESHARE, PARALLEL, COMMERCIAL.
#endif

/*
 * Timeshare systems assume a large number of users with medium number
 * of moderate size processes each, working on mostly unrelated files.
 * Little use of file-record-locking is made, and memory consumption
 * by processes outweighs the buf-cache.
 */

#ifdef	TIMESHARE

#if	defined(PARALLEL) || defined(COMMERCIAL)
	ERROR -- Specify only one of TIMESHARE, PARALLEL, COMMERCIAL.
#endif

#define	PROC_MULT	8	/* average # processes per user (rough) */
#define	INODE_MULT	1	/* INODE_MULT / INODE_DIV average # ... */
#define	INODE_DIV	1	/*	... inodes per process (rough) */
#define	FILE_MULT	8	/* FILE_MULT / FILE_DIV average # ... */
#define	FILE_DIV	5	/*	... files per process (rough) */
#define	MFILE_MULT	1	/* MFILE_MULT / MFILE_DIV average # ... */
#define	MFILE_DIV	2	/*	... mapped-file extents per process */
#define FLINO_MULT	1	/* average # lockable files per user (rough) */
#define FILCK_MULT	1	/* average # locks per lockable file (rough) */
#define	DEF_BUFPCT	25	/* default % free memory for buf-cache */
#define	DEF_AVGSZPROC	512	/* size(K) average size process */
#ifndef DEF_MINRS
#define	DEF_MINRS	64	/* size(K) default min Rset */
#endif

#endif	TIMESHARE

/*
 * Parallel/research systems assume a relatively small number of users
 * (maxusers <= 32 or so) each potentially using many large processes.  The
 * system must be able to execute a 30-process parallel application of nearly
 * full-sized processes without swapping due to limited page-table mapping
 * resource (Usrptmap[]).  Buf-cache size isn't as critical as available free
 * memory for the parallel program(s).  Little/no use of file-record-locking is
 * made.  Such systems may not be suitable for large parallel makes, since they
 * are tuned for large shared-memory programs (few large processes).
 *
 * 4K page size allows over-allocate Usrptmap (DEF_AVGSZPROC) without consuming
 * much kernel memory.
 */

#ifdef	PARALLEL

#if	defined(TIMESHARE) || defined(COMMERCIAL)
	ERROR -- Specify only one of TIMESHARE, PARALLEL, COMMERCIAL.
#endif

#define	PROC_MULT	16	/* average # processes per user (rough) */
#define	INODE_MULT	1	/* INODE_MULT / INODE_DIV average # ... */
#define	INODE_DIV	1	/*	... inodes per process (rough) */
#define	FILE_MULT	8	/* FILE_MULT / FILE_DIV average # ... */
#define	FILE_DIV	5	/*	... files per process (rough) */
#define	MFILE_MULT	1	/* MFILE_MULT / MFILE_DIV average # ... */
#define	MFILE_DIV	1	/*	... mapped-file extents per process */
#define FLINO_MULT	1	/* average # lockable files per user (rough) */
#define FILCK_MULT	1	/* average # locks per lockable file (rough) */
#define	DEF_BUFPCT	10	/* default % free memory for buf-cache */
#define	DEF_AVGSZPROC	(64*K)	/* size(K) average size process */
#ifndef DEF_MINRS
#define	DEF_MINRS	256	/* size(K) default min Rset */
#endif

#endif	PARALLEL

/*
 * Commercial systems assume a large number of users with smaller number
 * of processes each, working on many mostly small files.  Processes tend to
 * be fairly small and share a small number of executable binaries.  There
 * is greater use of file-locking and more buf-cache usually yields greater
 * performance.  Due to lots of sharing of pages in the binaries, a higher
 * minRS is reasonable.
 */

#ifdef	COMMERCIAL

#if	defined(TIMESHARE) || defined(PARALLEL) 
	ERROR -- Specify only one of TIMESHARE, PARALLEL, COMMERCIAL.
#endif

#define	PROC_MULT	5	/* average # processes per user (rough) */
#define	INODE_MULT	8	/* INODE_MULT / INODE_DIV average # ... */
#define	INODE_DIV	5	/*	... inodes per process (rough) */
#define	FILE_MULT	2	/* FILE_MULT / FILE_DIV average # ... */
#define	FILE_DIV	1	/*	... files per process (rough) */
#define	MFILE_MULT	1	/* MFILE_MULT / MFILE_DIV average # ... */
#define	MFILE_DIV	4	/*	... mapped-file extents per process */
#define FLINO_MULT	8	/* average # lockable files per user (rough) */
#define FILCK_MULT	8	/* average # locks per lockable file (rough) */
#define	DEF_BUFPCT	20	/* default % free memory for buf-cache */
#define	DEF_AVGSZPROC	400	/* size(K) average size process */
#ifndef DEF_MINRS
#define	DEF_MINRS	128	/* size(K) default min Rset */
#endif

#endif	COMMERCIAL

/*
 * Determine defaults for various table sizes and other parameters.
 * All of these can be overridden via "options" lines in the configuration
 * file; eg, "options NPROC=768".
 */

#ifndef	NPROC			/* # of process slots */
#define	NPROC	(20 + PROC_MULT * MAXUSERS)
#endif

#ifndef	NINODE			/* # concurrently active inodes */
#define	NINODE	((INODE_MULT * NPROC) / INODE_DIV + MAXUSERS + 48)
#endif

#ifndef	NFILE			/* # concurrently open files */
#define	NFILE	(FILE_MULT * (NPROC+16+MAXUSERS) / FILE_DIV + 32 + 2 * NETSLOP)
#endif

#ifndef	NMFILE			/* default # mapped file extents */
#define	NMFILE	(MFILE_MULT * NPROC / MFILE_DIV)
#endif

#ifndef	NCALLOUT		/* # active timeout events */
#define	NCALLOUT (16 + NPROC)
#endif

#ifndef	NCLIST			/* # C-list entries (tty subsystem buffers) */
#define	NCLIST	(100 + 16 * MAXUSERS)
#endif

#ifdef	INET
#define	NETSLOP	20		/* for the servers */
#else
#define	NETSLOP	0
#endif

#ifndef	NMBCLUSTERS		/* Max memory assignable to network buffers.
				 * mbufs will expand to this limit if necessary.
				 */
#define	NMBCLUSTERS	((256*K)/CLBYTES)
#endif

#ifndef	NMOUNT
#define	NMOUNT	64		/* default # mount table entries */
#endif

#ifndef	MAXUPRC
#define MAXUPRC 100		/* default # non root processes (per user) */
#endif

#ifndef	NUCRED
#define	NUCRED	NPROC		/* default # user-credential structures */
#endif

#ifndef	MAXSYMLINKS		/* Maximum number of symbolic links that may
				 * be expanded in a path name.  Should be set
				 * high enough to allow all legitimate uses,
				 * but halt infinite loops reasonably quickly.
				 */
#define	MAXSYMLINKS	8
#endif

#ifndef	NDEVNODE		/* Static # device vnodes (devtovp()) */
#define	NDEVNODE	0	/* 0 ==> determine heuristically */
#endif

#ifndef	BUFPCT			/* % initial free space to put in buf cache */
#define	BUFPCT		DEF_BUFPCT
#endif

#ifndef	MAXNOFILE		/* Max number file descriptors per process */
#define	MAXNOFILE	64	/* Default: std UNIX is 20; re-config to change */
#endif

#ifndef	NOFILETAB		/* # open-file table objects */
#define	NOFILETAB	NPROC	/* Default == NPROC */
#endif

#ifndef NUMBMAPCACHE
#define	NUMBMAPCACHE  	(50)
#endif

#ifdef QUOTA
#ifndef NDQUOT			/* # of currently activedquot struct */
#define	NDQUOT  	(NINODE + (MAXUSERS * NMOUNT) / 4)
#endif

#ifndef NDQHASH			/* # quota hash buckets */
#define	NDQHASH		((NDQUOT) / 3)
#endif

#ifndef DQ_FTIMEDEFAULT		/*
				 * Amount of time given a user before the soft
				 * limits for inodes are treated as hard limits
				 * (usually resulting in an allocation
				 * failure). These may be  modified by the
				 * quotactl system call with the Q_SETQLIM or
				 * Q_SETQUOTA command codes.
				 */
#define	DQ_FTIMEDEFAULT	(7 * 24 * 60 * 60)	/* 1 week */
#endif

#ifndef	DQ_BTIMEDEFAULT		/* time limit for disk blocks */
#define	DQ_BTIMEDEFAULT	(7 * 24 * 60 * 60)	/* 1 week */
#endif
#endif /* QUOTA */

#ifndef	CPUSPEED		/* approximate speed of cpu in VAX MIPS */
				/* scaled for an 100Mhz machine         */
#define	CPUSPEED	25	/* used for spin loops in various places */
#endif
#ifndef CPURATE
#define CPURATE		16	/* lowest cpu rate to used (Mhz) */
				/* must be smaller than the slowest cpu */
				/* rate or l.cpu_speed will be 0        */
#endif

#ifndef I486_CPUSPEED
#define I486_CPUSPEED   50
#endif I486_CPUSPEED

#ifndef INCR_PTSIZE		/* Incremental number of entries to */
				/* add to Usrptmap */
#define INCR_PTSIZE	0	/* Default: no extras needed */
#endif
#ifndef	AUTO_NICE
#define AUTO_NICE	1	/* auto nice by deafult */
#endif

int	hz		= HZ;
int	tick		= 1000000 / HZ;
int	tickadj		= 240000 / (60 * HZ);	/* can adjust 240ms in 60s */
int	wdt_timeout	= WDT_TIMEOUT;
int     ssm_wdt_timeout = SSM_WDT_TIMEOUT;
int	memintvl	= MEMINTVL * HZ;
struct	timezone tz	= { TIMEZONE, DST };

int	nproc		= NPROC;
int	ninode		= NINODE;
int	nfile		= NFILE;
int	ncallout	= NCALLOUT;
int	nclist		= NCLIST;
int	nmbclusters 	= NMBCLUSTERS;
int	nmount		= NMOUNT;
int	maxuprc		= MAXUPRC;
int	maxsymlinks	= MAXSYMLINKS;
int	nmfile		= NMFILE;
int	nucred		= NUCRED;
int     i486_lcpuspeed  = I486_CPUSPEED;
int	ndevnode	= NDEVNODE;
int	max_nofile	= MAXNOFILE;
int	nofile_tab	= NOFILETAB;
int	cpurate		= CPURATE;
int	lcpuspeed	= CPUSPEED;
int	NumBmapCache	= NUMBMAPCACHE;
int	incr_ptsize	= INCR_PTSIZE;
int	auto_nice	= AUTO_NICE;
#ifdef QUOTA
int	ndquot		= NDQUOT;
int	ndqhash		= NDQHASH;

int	dq_ftimedefault = DQ_FTIMEDEFAULT;
int	dq_btimedefault = DQ_BTIMEDEFAULT;
#else
/*
 * Define ndquot and ndqhash and set them to zero so that quotas
 * take no space if kernel was source configured with quotas, but
 * binary configured without them.
 */
#ifndef lint
int	ndquot = 0;			/* no dquot struct's */
int	ndqhash = 0;			/* no quota hash buckets */
int	dq_ftimedefault;
int	dq_btimedefault;
#endif /* lint */
#endif /* QUOTA */

/*
 * Tuneable paging parameters.
 * maxRS, vt_maxRS are adjusted when system comes up to insure fit in memory.
 */

#ifndef	AVG_SIZE_PROCESS			/* average process size (K) */
#define	AVG_SIZE_PROCESS	DEF_AVGSZPROC	/* for sizing Usrptmap[] */
#endif
int	avg_size_process = AVG_SIZE_PROCESS * K;

long	maxRS		= MAXADDR/CLBYTES;	/* max # clusters for Rset */
long	minRS		= 6;			/* min # clusters for rset */
long	maxRSslop	= (20*K)/NBPG;		/* slop for maxRS calculation */

struct	vm_tune	vmtune = {
	(DEF_MINRS*K)/CLBYTES,	/* (vt_minRS) min # clusters for Rset */
	MAXADDR/CLBYTES,	/* (vt_maxRS) max # clusters for Rset */
	(20*K)/NBPG,		/* (vt_RSexecslop) # HW pages slop in exec */
	4,			/* (vt_RSexecmult) Rset multipler */
	5,			/* (vt_RSexecdiv) Rset divider */
	0,			/* (vt_dirtylow) low dirty-list size */
	0,			/* (vt_dirtyhigh) high dirty-list size */
	32,			/* (vt_klout_look) pageout kluster look-ahead */
	1*HZ,			/* (vt_PFFvtime) ticks between PFF adjust */
	(8*K)/CLBYTES,		/* (vt_PFFdecr) pages to drop if PFF < PFFlow */
	2,			/* (vt_PFFlow) low PFF rate; <= PFFhigh */
	(20*K)/CLBYTES,		/* (vt_PFFincr) pages to add if PFF > PFFhigh */
	15,			/* (vt_PFFhigh) high PFF rate */
	0,			/* (vt_minfree) low free-list for swapping */
	0,			/* (vt_desfree) high free-list for swapping */
	0,			/* (vt_maxdirty) max dirty-list before swap */
};

bool_t	root_vm_setrs = 1;	/* must be root to do vm_ctl(VM_SETRS) */
bool_t	root_vm_swapable = 1;	/* must be root to do vm_ctl(VM_SWAPABLE,0) */
bool_t	root_vm_pffable = 1;	/* must be root to do vm_ctl(VM_PFFABLE,0) */
bool_t	root_prio_noage = 1;	/* must be root to do proc_ctl(PRIO_AGE,x,0) */

/*
 * Fork-map parameters; determine amount of memory the system will dedicate
 * to concurrent forks by mult/div initial free-space.
 */

long	forkmapmult =	2;	/* multiplier */
long	forkmapdiv =	3;	/* divisor */

/*
 * Variables initialized here to zero are initialized at bootstrap time
 * to values dependent on memory size.
 */

int	nbuf = 0;		/* number of buffer cache headers */
int	nswbuf = 0;		/* number of swap buffer headers */
int	bufpages = 0;		/* # clusters allocated to buffer cache */
int	bufhsz = 0;		/* # buffer cache hash buckets */
int	buf_hash_mult = 1;	/* bufhsz = nbuf * ... */
int	buf_hash_div = 2;	/*	... buf_hash_mult / buf_hash_div */
int	bufpct = BUFPCT;	/* pct of free mem to allocate to bufpages */

int	mmreg_maxb = 10;	/* max # buffers to use for mapped-file sync */
int	pageout_maxb = 10;	/* max # buffers to use for pageouts */

int	ummap_max_hole = (64*K)/NBPG;	/* max u_mmap concat hole */

/*
 * light_show decides which LED's are used to display system activity.
 * B8k's can only display on the processor board LED's; B21k's can display
 * on the front-panel and processor board LED's.  light_show == 1 displays
 * in one place (front-panel if B21k); light_show == 2 displays in both places
 * if possible; light_show == 0 doesn't flash at all (quite boring).
 */

int	light_show = 1;		/* display on most useful place (==1) */

#define DK_NSTATDRIVE (48)	/* Number of drives we can keep stats on */
struct	dk dk[DK_NSTATDRIVE];	/* Array of stat structures for drives */
int	dk_ndrives = sizeof dk/sizeof dk[0];	/* max number of drives */
int	dk_nxdrive = 0;		/* next drive number, index into dk[] */

/*
 * Non-zero resphysmem constrains amount of physical memory the system will
 * use, reserving some for special purpose drivers, accelerators, etc.
 * Physical address of reserved memory is held in `topmem'.
 * Must be a multiple of CLBYTES.
 */

#ifndef	RESPHYSMEM
#define	RESPHYSMEM	0	/* default: none reserved */
#endif

int	resphysmem = RESPHYSMEM;

/*
 * KL2PT sizes.
 */

short	Mbmapsize		= NMBCLUSTERS*CLSIZE;	/* mbuf map */

/*
 * default stack limit
 */

#ifndef	DEFSSIZ
#define	DEFSSIZ	1024		/* default is 1024K bytes */
#endif

int	defssiz = DEFSSIZ*K;

/*
 * structures for record locking implementation
 */

#ifndef	NFLINO			/* # of file headers */
#define NFLINO	 ((MAXUSERS*FLINO_MULT) + 50)
#endif
#ifndef	NFILCK			/* # of record locks */
#define NFILCK	(NFLINO*FILCK_MULT)
#endif

struct	flino	flinotab[NFLINO];
struct	filock	flox[NFILCK];
struct	flckinfo flckinfo = { NFILCK, NFLINO };

#ifndef	SVMESG

msgsys() {nosys();}
msginit() {return(0);}

#else	SVMESG

#ifndef	IPC_ALLOC
#include "../h/ipc.h"
#endif
#include "../h/msg.h"

#ifndef	MSGMAP
#define	MSGMAP	100	/* number of map structure entries */
#endif
#ifndef	MSGMAX
#define	MSGMAX	8192	/* max message size (bytes) 	*/
#endif
#ifndef	MSGMNB
#define	MSGMNB	16384	/* max number of bytes on a queue */
#endif
#ifndef	MSGMNI
#define	MSGMNI	50	/* number of message queue identifiers */
#endif
#ifndef	MSGSSZ
#define	MSGSSZ	8	/* size of a message segment */
#endif
#ifndef	MSGTQL
#define	MSGTQL	40	/* number of system message headers */
#endif
#ifndef	MSGSEG
#define	MSGSEG	1024	/* number of message segments */
#endif

char	Bmsg[MSGSEG*MSGSSZ];

struct map	msgmap[MSGMAP];
struct msqid_ds	msgque[MSGMNI];
struct msg	msgh[MSGTQL];
struct msginfo	msginfo = {
	MSGMAP,
	MSGMAX,
	MSGMNB,
	MSGMNI,
	MSGSSZ,
	MSGTQL,
	MSGSEG
};

#endif	SVMESG

#ifndef	SVSEMA

semsys() {nosys();}
seminit() {}
semexit() {}

#else	SVSEMA

#ifndef	IPC_ALLOC
#include "../h/ipc.h"
#endif
#include "../h/sem.h"

#ifndef	SEMMAP
#define	SEMMAP	10
#endif
#ifndef	SEMMNI
#define	SEMMNI	10
#endif
#ifndef	SEMMNS
#define	SEMMNS	60
#endif
#ifndef	SEMMNU
#define	SEMMNU	30
#endif
#ifndef	SEMMSL
#define	SEMMSL	25
#endif
#ifndef	SEMOPM
#define	SEMOPM	10
#endif
#ifndef	SEMUME
#define	SEMUME	10
#endif
#ifndef	SEMVMX
#define	SEMVMX	32767
#endif
#ifndef	SEMAEM
#define	SEMAEM	16384
#endif

struct semid_ds	sema[SEMMNI];
struct sem	sem[SEMMNS];
struct map	semmap[SEMMAP];
struct	sem_undo	*sem_undo[NPROC];
#define	SEMUSZ	(sizeof(struct sem_undo)+sizeof(struct undo)*SEMUME)
int	semu[((SEMUSZ*SEMMNU)+NBPW-1)/NBPW];
union {
	short		semvals[SEMMSL];
	struct semid_ds	ds;
	struct sembuf	semops[SEMOPM];
}	semtmp;

struct	seminfo seminfo = {
	SEMMAP,
	SEMMNI,
	SEMMNS,
	SEMMNU,
	SEMMSL,
	SEMOPM,
	SEMUME,
	SEMUSZ,
	SEMVMX,
	SEMAEM
};

#endif	SVSEMA

#ifndef	SVCHOWN
#define	SVCHOWN		0
#endif
#ifndef	SVACCT
#define	SVACCT		0
#endif

int	allow_chown = SVCHOWN;
int	sys5acct = SVACCT;
int	cmask = CMASK;

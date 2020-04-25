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
 * $Header: param.h 2.20 1991/09/20 22:13:52 $
 *
 * param.h
 *	Implementation parameters.
 */

/* $Log: param.h,v $
 *
 *
 *
 */

/*
 * Machine type dependent parameters.
 */

#ifdef KERNEL
#include "../machine/param.h"
#else
#include <machine/param.h>
#endif

#define	NPTEPG		(NBPG/(sizeof (struct pte)))	/* # pte's in HW page */
#define	NPTECL		(NPTEPG*CLSIZE)			/* # pte's in cluster */

/*
 * Machine-independent constants
 */
#ifndef	KERNEL
	/*
	 *	for backward compatibility, we define this value
	 *	for use by the following non-kernel utilities.
	 *	  /etc/df.c /etc/edquota /etc/mount /etc/umount
	 *	  /etc/quotaon /etc/fsck stand-alone 
	 *	see conf/param.c for 'real' kernel definition
	 */
#define	NMOUNT	128		/* number of mountable file systems */
#endif

#define	MAXLINK	32767		/* max links to a file or directory */
#define	MSWAPX	15		/* pseudo mount table index for swapdev */
#define	NOFILE	20		/* default max open files per process --
				   see setdtablesize(2) for real max */
#define MAXUID	60000		/* SVAE imposed uid limit (also in 5include
				  param.h) */
#define	CANBSIZ	256		/* max size of typewriter line */
#define	NCARGS	((10240+CLOFSET)&~CLOFSET) /* # characters in exec arglist */
#define	NGROUPS	16		/* max number groups */
#define MAXHOSTNAMELEN	64	/* maximum hostname size*/

#define	NOGROUP	-1		/* marker for empty group set member */

/*
 * Priorities
 */
#define	PSWP	0
#define	PRSLOCK	7		/* lock resource list */
#define	PRSWAIT	8		/* wait for resource */
#define	PINOD	10
#define	PVNOD	PINOD
#define	PRIBIO	20
#define PVFS	23
#define	PRIUBA	24
#define	PZERO	25
#define	PPIPE	26
#define	PWAIT	30
#define	PLOCK	35
#define	PSLEP	40
#define	PUSER	50
#define	PIDLE	127

#define	NZERO	20

/*
 * Signals
 */
#ifdef KERNEL
#include "../h/signal.h"
#else
#include <signal.h>
#endif

/*
 * Fundamental constants of the implementation.
 */
#define	NBBY	8		/* number of bits in a byte */
#define	NBPW	sizeof(int)	/* number of bytes in an integer */

#define	NULL	0
#define	CMASK	0		/* default mask for file creation */
#define	NODEV	(dev_t)(-1)

/*
 * Clustering of hardware pages on machines with ridiculously small
 * page sizes is done here.  The paging subsystem deals with units of
 * CLSIZE pte's describing NBPG (from vm.h) pages each...
 *
 * NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE
 */
#define	CLBYTES		(CLSIZE*NBPG)
#define	CLOFSET		(CLSIZE*NBPG-1)	/* for clusters, like PGOFSET */
#define	CLSHIFT		(PGSHIFT+CLSIZELOG2)

#if CLSIZE==1
#define	clbase(i)	(i)
#define	clrnd(i)	(i)
#else
/* give the base virtual address (first of CLSIZE) */
#define	clbase(i)	((i) &~ (CLSIZE-1))
/* round a number of clicks up to a whole cluster */
#define	clrnd(i)	(((i) + (CLSIZE-1)) &~ (CLSIZE-1))
#endif

#define	CBSIZE	60		/* number of chars in a clist block */
#define	CROUND	0x3F		/* clist rounding; sizeof(int *) + CBSIZE -1*/

#ifndef KERNEL
#include	<sys/types.h>
#else
#ifndef LOCORE
#include	"../h/types.h"
#endif
#endif

/*
 * File system parameters and macros.
 *
 * The file system is made out of blocks of at most MAXBSIZE units,
 * with smaller units (fragments) only in the last direct block.
 * MAXBSIZE primarily determines the size of buffers in the buffer
 * pool. It may be made larger without any effect on existing
 * file systems; however making it smaller make make some file
 * systems unmountable.
 *
 * Note that the blocked devices are assumed to have DEV_BSIZE
 * "sectors" and that fragments must be some multiple of this size.
 * Block devices are read in BLKDEV_IOSIZE units. This number must
 * be a power of two and in the range of
 *	DEV_BSIZE <= BLKDEV_IOSIZE <= MAXBSIZE
 * This size has no effect upon the file system, but is usually set
 * to the block size of the root file system, so as to maximize the
 * speed of ``fsck''.
 */

#define	MAXBSIZE	8192
#define	DEV_BSIZE	512
#define	DEV_BSHIFT	9		/* log2(DEV_BSIZE) */
#define BLKDEV_IOSIZE	2048
#define MAXFRAG 	8

#define	btodb(bytes)	 		/* calculates (bytes / DEV_BSIZE) */ \
	((unsigned)(bytes) >> DEV_BSHIFT)
#define	dbtob(db)			/* calculates (db * DEV_BSIZE) */ \
	((unsigned)(db) << DEV_BSHIFT)

/*
 * Map a ``block device block'' to a file system block.
 * This should be device dependent, and will be after we
 * add an entry to cdevsw for that purpose.  For now though
 * just use DEV_BSIZE.
 */
#define	bdbtofsb(bn)	((bn) / (BLKDEV_IOSIZE/DEV_BSIZE))

/*
 * MAXPATHLEN defines the longest permissible path length
 * before and after expanding symbolic links. It is used to allocate
 * a temporary buffer from the buffer pool in which to do the
 * name expansion, hence should be a power of two, and must
 * be less than or equal to MAXBSIZE.
 * MAXCSYMLEN defines maximum length of a conditional symbolic link.
 */
#define MAXPATHLEN	1024
#define MAXCSYMLEN	MAXPATHLEN

/*
 * bit map related macros
 */
#define	setbit(a,i)	((a)[(i)/NBBY] |= 1<<((i)%NBBY))
#define	clrbit(a,i)	((a)[(i)/NBBY] &= ~(1<<((i)%NBBY)))
#define	isset(a,i)	((a)[(i)/NBBY] & (1<<((i)%NBBY)))
#define	isclr(a,i)	(((a)[(i)/NBBY] & (1<<((i)%NBBY))) == 0)

/*
 * Macros for fast min/max.
 */
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))

/*
 * Macros for counting and rounding.
 */
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

/*
 * sanity macro.
 */

#ifdef	KERNEL
# ifdef	NOASSERTCHECK
#  define	ASSERT(c,s)
# else
#  define	ASSERT(c,s)	if (c) /* skip */; else panic(s)
# endif

#ifdef	DEBUG
#define	ASSERT_DEBUG(c,s)	ASSERT(c,s)
#else
#define	ASSERT_DEBUG(c,s)
#endif	DEBUG

#endif	KERNEL

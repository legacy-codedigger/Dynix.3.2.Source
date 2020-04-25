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
 * $Header: buf.h 2.6 1991/04/16 23:51:01 $
 *
 * buf.h
 *	Buffer header definitions.
 *
 * The header for buffers in the buffer pool and otherwise used to
 * describe a block i/o request is given here.  The routines which
 * manipulate these things are given in ufs_bio.c and ufs_machdep.c
 */

/* $Log: buf.h,v $
 *
 */

/*
 * Each buffer in the pool is usually doubly linked into 2 lists:
 * hashed into a chain by <dev,blkno> so it can be located in the cache,
 * and (usually) on (one of several) queues.  These lists are circular and
 * doubly linked for easy removal.
 *
 * There are currently four queues for buffers:
 *	one for buffers which must be kept permanently (super blocks)
 * 	one for buffers containing ``useful'' information (the cache)
 *	one for buffers containing ``non-useful'' information
 *	one for buffers containing no pages (eg, empty)
 *
 * The middle two queues contain the buffers which are available for
 * reallocation, are kept in lru order.  When not on one of these queues,
 * the buffers are ``checked out'' to drivers which use the available list
 * pointers to keep track of them in their i/o active queues.
 */

/*
 * Bufhd structures used at the head of the hashed buffer queues.
 * We only need three words for these, so this abbreviated
 * definition saves some space.
 */
struct bufhd
{
	long	b_flags;		/* see defines below */
	struct	buf *b_forw, *b_back;	/* fwd/bkwd pointer in chain */
};
struct buf
{
	long	b_flags;		/* too much goes here to describe */
	struct	buf *b_forw, *b_back;	/* hash chain (2 way street) */
	struct	buf *av_forw, *av_back;	/* position on free list if not BUSY */
#define	b_actf	av_forw			/* alternate names for driver queue */
#define	b_actl	av_back			/*    head - isn't history wonderful */
	long	b_bcount;		/* transfer count */
	long	b_bufsize;		/* size of allocated buffer */
#define	b_active b_bcount		/* driver queue head: drive active */
	short	b_error;		/* returned after I/O */
	dev_t	b_dev;			/* major+minor device name */
	char	b_iotype;		/* type of IO -- see below */
	union {
	    caddr_t	b_addr;		/* low order core address */
	    int		*b_words;	/* words for clearing */
	    struct fs	*b_fs;		/* superblocks */
	    struct csum	*b_cs;		/* superblock summary information */
	    struct cg	*b_cg;		/* cylinder group block */
	    struct dinode *b_dino;	/* ilist */
	    struct pte	*b_pte;		/* pte-based IO */
	    daddr_t	*b_daddr;	/* indirect block */
	} b_un;
	daddr_t	b_blkno;		/* block # on device */
 	struct	vnode *b_vp;		/* vnode associated with block */
	long	b_resid;		/* words not transferred after error */
#define	b_errcnt b_resid		/* while i/o in progress: # retries */
	struct  proc *b_proc;		/* proc doing physical or swap I/O */
	int	(*b_iodone)();		/* function called by iodone */
	sema_t	b_iowait;		/* place to wait for IO completion */
	sema_t	b_alloc;		/* place to wait to allocate the buf */
};

#define	BQUEUES		4		/* number of free buffer queues */

#define	BQ_LOCKED	0		/* super-blocks (not used in 4.2) */
#define	BQ_LRU		1		/* lru, useful buffers */
#define	BQ_AGE		2		/* rubbish */
#define	BQ_EMPTY	3		/* buffer headers with no memory */

#ifdef	KERNEL
#define	MIN_BUFHSZ	64		/* min # buffer hash buckets */
#define RND	(MAXBSIZE/DEV_BSIZE)	/* hash bucket "rounding" */
#define	BUFHASH(dvp, dblkno)	\
	((struct buf *) &bufhash[((u_int)(dvp)+(((int)(dblkno))/RND)) % bufhsz])

extern	struct	buf *buf;		/* the buffer pool itself */
extern	char	*buffers;		/* virt addr of buffer memory */
extern	int	nbuf;			/* # buffer headers */
extern	int	bufpages;		/* # memory pages in the buffer pool */
extern	int	bufhsz;			/* # buffer cache hash buckets */

extern	int	nswbuf;			/* # swap buffer headers */
extern	struct	buf *swbuf;		/* swap I/O headers */
extern	struct	buf *bswlist;		/* head of free swap header list */
extern	lock_t	swbuf_mutex;		/* lock for the list */
extern	sema_t	swbuf_wait;		/* waiting sema for free headers */
#ifdef	PERFSTAT
extern	int	numswfree;		/* current # free swap buffers */
extern	int	minswfree;		/* min # free swap headers ever */
#endif	PERFSTAT

extern	struct	bufhd *bufhash;		/* heads of hash lists */
extern	struct	bufhd *bufhashBUFHSZ;	/* limit of bufhash array */
extern	struct	buf bfreelist[BQUEUES];	/* heads of available lists */
extern	lock_t	buf_lists;		/* lock for the lists */
extern	sema_t	buf_wait;		/* waiting sema for free headers */

struct	buf *alloc();
struct	buf *realloccg();
struct	buf *baddr();
struct	buf *getblk();
struct	buf *geteblk();
struct	buf *getnewbuf();
struct	buf *bread();
struct	buf *breada();

unsigned minphys();
#endif

/*
 * IO type definitions (b_iotype).  The 'b_addr' field is defined based on
 * the value of b_iotype:
 *
 * B_FILIO:	kernel virtual address of cache-buffer memory
 * B_RAWIO:	user virtual address of buffer
 * B_PTEIO:	points at 1st pte describing memory involved with the transfer
 * B_PTBIO:	B_PTEIO, but allow non-cluster aligned start/count
 */

#define	B_FILIO		0		/* file-sys IO: cache buffers */
#define	B_RAWIO		1		/* RAW IO: arbitrary user addr space */
#define	B_PTEIO		2		/* pte IO: swap, page */
#define	B_PTBIO		3		/* Page-Table IO: allows unaligned */

/*
 * These flags are kept in b_flags.
 *
 * B_PHYS, B_UAREA, B_PAGET are redundant with B_PTEIO, but kept in case
 * drivers need them.
 */

#define	B_WRITE		0x00000000	/* non-read pseudo-flag */
#define	B_READ		0x00000001	/* read when I/O occurs */
#define	B_ERROR		0x00000004	/* transaction aborted */
#define	B_PHYS		0x00000010	/* physical IO */
#define	B_AGE		0x00000080	/* delayed write for correct aging */
#define	B_ASYNC		0x00000100	/* don't wait for I/O completion */
#define	B_DELWRI	0x00000200	/* write at exit of avail list */
#define	B_TAPE		0x00000400	/* this is a magtape (no bdwrite) */
#define	B_UAREA		0x00000800	/* add u-area to a swap operation */
#define	B_PAGET		0x00001000	/* page in/out of page table space */
#define	B_DIRTY		0x00002000	/* dirty page to be pushed out async */
#define	B_PGIN		0x00004000	/* pagein op, so swap() can count it */
#define	B_INVAL		0x00010000	/* does not contain valid info  */
#define	B_HEAD		0x00040000	/* a buffer header, not a buffer */
#define	B_CALL		0x00200000	/* call b_iodone from iodone */
#define	B_IOCTL		0x00400000	/* an ioctl request (driver local) */

#define	B_REALLOC	0x00800000	/* buffer involved with brealloc */
#define B_NOTREF	0x01000000	/* perf - on until used from breada */
#define	B_NOCLR		0x02000000	/* pseudo flag, argument to bmap */

#define	B_EXPRESS	0x04000000	/* buf blk is FIFO instead of sweep */
					/* (not currently used */
#define	B_SYNC		0x08000000	/* pseudo flag, argument to bmap */
#define B_NOCACHE	0x10000000	/* don't cache block when released */
					/* (not currently used */

/*
 * Macros to ask questions about buf status.  These replace use of B_DONE,
 * B_BUSY, B_WANTED.
 */

#define	BIODONE(bp)	sema_count(&(bp)->b_iowait)
#define	BAVAIL(bp)	sema_avail(&(bp)->b_alloc)
#define	BWANTED(bp)	blocked_sema(&(bp)->b_alloc)

/*
 * Insq/Remq for the buffer hash lists.
 */
#define	bhashself(bp) { \
	(bp)->b_back = (bp)->b_forw = (bp); \
}
#define	bremhash(bp) { \
	(bp)->b_back->b_forw = (bp)->b_forw; \
	(bp)->b_forw->b_back = (bp)->b_back; \
}
#define	binshash(bp, dp) { \
	(bp)->b_forw = (dp)->b_forw; \
	(bp)->b_back = (dp); \
	(dp)->b_forw->b_back = (bp); \
	(dp)->b_forw = (bp); \
}

/*
 * Insq/Remq for the buffer free lists.
 */
#define	bremfree(bp) { \
	(bp)->av_back->av_forw = (bp)->av_forw; \
	(bp)->av_forw->av_back = (bp)->av_back; \
}
#define	binsheadfree(bp, dp) { \
	(dp)->av_forw->av_back = (bp); \
	(bp)->av_forw = (dp)->av_forw; \
	(dp)->av_forw = (bp); \
	(bp)->av_back = (dp); \
}
#define	binstailfree(bp, dp) { \
	(dp)->av_back->av_forw = (bp); \
	(bp)->av_back = (dp)->av_back; \
	(dp)->av_back = (bp); \
	(bp)->av_forw = (dp); \
}

/*
 * Take a buffer off the free list it's on and
 * mark it as being use (B_BUSY) by a device.
 *
 * This assumes that notavail() is called while the lists (hash+free) are
 * locked, and it is known that no other process is trying to wait on the
 * given buf header.
 */
#define	notavail(bp) { \
	bremfree(bp); \
	sema_count(&(bp)->b_alloc) = 0; \
}

#define	iodone	biodone				/* backwards compatibility */
#define	iowait	biowait				/* backwards compatibility */

/*
 * Zero out a buffer's data portion.
 */
#define	clrbuf(bp) { \
	bzero(bp->b_un.b_addr, (unsigned)bp->b_bcount); \
	bp->b_resid = 0; \
}

/*
 * Release space associated with a buffer.
 */
#define	bfree(bp)	{ (bp)->b_bcount = 0; }

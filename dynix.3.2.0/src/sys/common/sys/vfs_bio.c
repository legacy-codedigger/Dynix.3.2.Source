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
static	char	rcsid[] = "$Header: vfs_bio.c 2.23 1991/06/06 21:28:26 $";
#endif

/*
 * vfs_bio.c
 *	File-System buffer cache management.
 *
 * Buffer usage model is that once allocated (getblk()/etc) the "owner"
 * of the buffer has complete control over flags/etc.  Other processes
 * may sleep on the sema's for allocation or IO wait.  Once a buf-header
 * given to driver thru bdevsw[], we must not touch it other than waiting
 * on semaphores.
 */

/* $Log: vfs_bio.c,v $
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/kernel.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/vm.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"
#include "../machine/plocal.h"			/* instrumentation */
#include "../machine/gate.h"			/* instrumentation */

/*
 * Define the hash and free lists.
 */

struct	bufhd	*bufhash;			/* heads of hash lists */
struct	bufhd	*bufhashBUFHSZ;			/* limit of bufhash array */
struct	buf	bfreelist[BQUEUES];		/* heads of available lists */
lock_t		buf_lists;			/* the locker for both lists */
sema_t		buf_wait;			/* wait here for free headers */
sema_t		buf_invals;			/* binval/bflush interlock */

/*
 * Flags for getblk() option parameter.  No flags ==> normal behavior.
 */

#define	BG_NOHIT	1			/* don't get if in cache */
#define	BG_NOMISS	2			/* don't get if not in cache */

/*
 * Macros for NFS to control ref counts for vnodes of buffers on remote devs.
 * For performance, we NOP these if we are not running NFS.
 */

#ifdef	NFS
#define	CVN_HOLD(VP)	if (!((VP)->v_flag & VNBACCT)) VN_HOLD(VP)
#define	CVN_RELE(VP)	if ((VP)!=NULLVP && !((VP)->v_flag&VNBACCT)) VN_RELE(VP)
#else
#define	CVN_HOLD(VP)
#define	CVN_RELE(VP)
#endif	NFS

/*
 * bhinit()
 *	Initialize hash links for buffers.
 */

bhinit()
{
	register struct bufhd *bp;

	for (bp = bufhash; bp < bufhashBUFHSZ; bp++)
		bp->b_forw = bp->b_back = (struct buf *)bp;
}

/*
 * binit()
 *	Initialize the buffer I/O system (called from main()).
 *
 * This is done by initializing the various locks and semas, then freeing
 * all buffers and setting all device buffer lists to empty.
 *
 * The gates G_BUFMIN->G_BUFMAX are distributed among the buf-header
 * semaphores (b_iowait, b_alloc).
 */

binit()
{
	register struct buf *bp;
	register struct buf *dp;
	register int i;
	register int gateno;
	int	base;
	int	residual;

	init_lock(&buf_lists, G_BUFLISTS);		/* list lock */
	init_sema(&buf_wait, 0, 0, G_BUFWAIT);		/* waiting sema */

	init_sema(&buf_invals, 1, 0, G_BUFWAIT);	/* inval sema */

	for (dp = bfreelist; dp < &bfreelist[BQUEUES]; dp++) {
		dp->b_forw = dp->b_back = dp->av_forw = dp->av_back = dp;
		dp->b_flags = B_HEAD;
	}

	gateno = 0;
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		bp = &buf[i];
		bp->b_dev = NODEV;
		bp->b_bcount = 0;
		bp->b_un.b_addr = buffers + i * MAXBSIZE;
		if (i < residual)
			bp->b_bufsize = (base + 1) * CLBYTES;
		else
			bp->b_bufsize = base * CLBYTES;
		bhashself(bp);
		bp->b_flags = B_INVAL;
		init_sema(&bp->b_iowait, 0, 0,
			(gate_t)(G_BUFMIN + gateno++ % (G_BUFMAX-G_BUFMIN+1)));
		init_sema(&bp->b_alloc, 0, 0,
			(gate_t)(G_BUFMIN + gateno++ % (G_BUFMAX-G_BUFMIN+1)));
		brelse(bp);
	}
#ifdef	lint
	lint_ref_int(gateno);
#endif	lint
}

/*
 * bufinit()
 *	Utility for drivers to init their local buf-headers.
 *
 * Currently assumes same gate for both b_iodone and b_alloc semaphores.
 */

/*ARGSUSED*/
bufinit(bp, gate)
	struct buf	*bp;
	gate_t		gate;
{
	init_sema(&bp->b_iowait, 0, 0, gate);		/* IO not done */
	init_sema(&bp->b_alloc,  1, 0, gate);		/* available */
}

/*
 * bufalloc()
 *	Allocate a (driver private) buffer.
 *
 * Provides an interface independent of sema's vs sleep/wakeup, and
 * removes complication from driver.
 */

bufalloc(bp)
	struct buf	*bp;
{
	p_sema(&bp->b_alloc, PRIBIO+1);
}

/*
 * buffree()
 *	Free/release a (driver private) buffer.  Dual of bufalloc().
 */

buffree(bp)
	struct buf	*bp;
{
	v_sema(&bp->b_alloc);
}

/*
 * bread()
 *	Read in (if necessary) the block and return a buffer pointer.
 */

struct buf *
bread(vp, blkno, size)
	struct	vnode *vp;
	daddr_t	blkno;
	int	size;
{
	register struct buf *bp;

	ASSERT(size != 0, "bread: size 0");
	/*
	 *+ The filesystem code tried to read a disk block
	 *+ of size 0 into a buffer cache buffer.
	 */
 	l.cnt.v_lreads++;
	bp = getblk(vp, blkno, size, 0);
	if (BIODONE(bp)) {
 		if (bp->b_flags&B_NOTREF) {
 			bp->b_flags &= ~B_NOTREF;
 			l.cnt.v_bhitra++;
 		} else
 			l.cnt.v_bhitru++;
		return (bp);
	}
	bp->b_flags |= B_READ;
	ASSERT_DEBUG(bp->b_bcount <= bp->b_bufsize, "bread: bcount");
	VOP_STRATEGY(bp);
 	l.cnt.v_rcount++;
	u.u_ru.ru_inblock++;			/* pay for read */
	biowait(bp);
	ASSERT_DEBUG(bp->b_bcount >= size, "bread: size");
	return (bp);
}

/*
 * breada()
 *	Read in the block, like bread, but also start I/O on the
 *	read-ahead block (which is not allocated to the caller)
 */

struct buf *
breada(vp, blkno, size, rablkno, rabsize)
	struct	vnode *vp;
	daddr_t	blkno;
	int	size;
	daddr_t	rablkno;
	int	rabsize;
{
	register struct buf *bp;
	register struct buf *rabp;

	/*
	 * Get required block, starting IO if not already in memory.
	 */

 	l.cnt.v_lreads++;
	bp = getblk(vp, blkno, size, 0);
	if (BIODONE(bp) == 0) {
		bp->b_flags |= B_READ;
		ASSERT_DEBUG(bp->b_bcount<=bp->b_bufsize,"breada: bp b_bcount");
		VOP_STRATEGY(bp);
 		l.cnt.v_rcount++;
		u.u_ru.ru_inblock++;		/* pay for read */
	} else {
 		if (bp->b_flags&B_NOTREF) {
 			bp->b_flags &= ~B_NOTREF;
 			l.cnt.v_bhitra++;
 		} else
 			l.cnt.v_bhitru++;
 	}

	/*
	 * If there's a read-ahead block, start i/o
	 * on it also (as above), if it's not already in memory.
	 * Call to getblk() below returns zero if block is in memory.
	 */

	if (rablkno) {
 		l.cnt.v_racount++;
		rabp = getblk(vp, rablkno, rabsize, BG_NOHIT);
		if (rabp) {
			ASSERT_DEBUG(BIODONE(rabp) == 0, "breada: ra synch");
			ASSERT_DEBUG(rabp->b_bcount <= rabp->b_bufsize,
						"breada: rabp b_bcount");
			rabp->b_flags |= B_READ|B_ASYNC|B_NOTREF;
			VOP_STRATEGY(rabp);
 			l.cnt.v_rcount++;
			u.u_ru.ru_inblock++;		/* pay in advance */
		} else {
 			l.cnt.v_rainbc++;
		}
	}

	/*
	 * If IO not done it was started above, so wait for it.
	 */

	if (BIODONE(bp) == 0)
		biowait(bp);
	ASSERT_DEBUG(bp->b_bcount >= size, "breada: size");
	return (bp);
}

/*
 * bwrite()
 *	Write the buffer, waiting for completion.
 *	Then release the buffer.
 */

bwrite(bp)
	register struct buf *bp;
{
	register int flags;

 	l.cnt.v_lwrites++;
	flags = bp->b_flags;
	bp->b_flags &= ~(B_READ | B_ERROR | B_DELWRI);
	if ((flags&B_DELWRI) == 0)
		u.u_ru.ru_oublock++;		/* none paid yet */
	else
		bp->b_flags |= B_AGE;		/* make it more recycleable */
	ASSERT_DEBUG(bp->b_bcount <= bp->b_bufsize, "bwrite: b_count");
	BIODONE(bp) = 0;			/* new IO starting */
	VOP_STRATEGY(bp);
 	l.cnt.v_wcount++;

	/*
	 * If the write was synchronous, then await i/o completion.
	 * If the write was "delayed", then we already arranged to
	 * make the buffer more allocatable (eg, brelse will put it
	 * on BQ_AGE when IO finishes).
	 */

	if ((flags&B_ASYNC) == 0) {
		biowait(bp);
		brelse(bp);
	}
}

/*
 * bdwrite()
 *	Release buffer, marking for delayed writing.
 *
 * Release the buffer, marking it so that if it is grabbed
 * for another purpose it will be written out before being
 * given up (e.g. when writing a partial block where it is
 * assumed that another write for the same block will soon follow).
 * This can't be done for magtape, since writes must be done
 * in the same order as requested.
 */

bdwrite(bp)
	register struct buf *bp;
{
 	l.cnt.v_lwrites++;
	if ((bp->b_flags&B_DELWRI) == 0)
		u.u_ru.ru_oublock++;		/* none paid yet */
	ASSERT_DEBUG(bp->b_vp != NULLVP, "bdwrite: NULLVP");
	if (bp->b_vp->v_flag & VBTAPE)
		bawrite(bp);
	else {
		bp->b_flags |= B_DELWRI;
		BIODONE(bp) = 1;		/* later getblk() thinks done */
		brelse(bp);
	}
}

/*
 * bawrite()
 *	Release the buffer, start I/O on it, but don't wait for completion.
 */

bawrite(bp)
	register struct buf *bp;
{
	bp->b_flags |= B_ASYNC;
	bwrite(bp);
}

/*
 * brelse()
 *	Release the buffer, with no I/O implied.
 *
 * This can be called from an interrupt handler, indirect thru biodone().
 *
 * Buffer is given to a waiter, else freed.
 * Error or invalid buffers are made non-findable in searches,
 * and placed in head of likely to allocate list.
 *
 * NFS use calls ONLY from process context; thus CVN_RELE(oldvp) for
 * B_ERROR|B_INVAL buffer is ok.
 *
 * References to B_LOCKED have been removed, since this is unused
 * anywhere in the remainder of the kernel.  This makes bfreelist[BQ_LOCKED]
 * unused other than termination condition in buffer allocation.  We'll
 * keep it around in case the need comes back.
 */

brelse(bp)
	register struct buf *bp;
{
	register struct buf *flist;
	register spl_t	s;
#ifdef	NFS
	struct	vnode	*oldvp = NULLVP;
#endif	NFS

	/*
	 * If someone's waiting for the buffer wake 'em up, and
	 * don't actually release the buffer.
	 *
	 * Have to lock buf_lists to insure stability of state.
	 */

	s = p_lock(&buf_lists, SPLBUF);

	if (blocked_sema(&bp->b_alloc)) {
		bp->b_flags &= ~(B_ASYNC|B_AGE);
		/*
		 * Ok to v_lock first, since already decided there is
		 * a process waiting, and there are no signals wakeups.
		 * Most that can occur is more processes queue for same
		 * buffer.  If invalid, must zap b_vp so getblk() and others
		 * don't see it and queue more waiters (avoid "livelock"); ok
		 * to leave on hash-chain, when finally do release it will
		 * remove.
		 */
		if (bp->b_flags & (B_ERROR|B_INVAL)) {
#ifdef	NFS
			oldvp = bp->b_vp;
#endif	NFS
			bp->b_vp = NULLVP;
		}
		v_lock(&buf_lists, s);
		v_sema(&bp->b_alloc);
		CVN_RELE(oldvp);
		return;
	}

	/*
	 * If this is a no cache buffer then mark it as B_INVAL
	 * to get it placed on the front of the free list.
	 */
	if (bp->b_flags & B_NOCACHE) {
		bp->b_flags |= B_INVAL;
	}

	/*
	 * Stick the buffer back on a free list.
	 * Note: to declare "available" sufficient to set b_alloc counter,
	 * since we know no-one is waiting for the buf, and we hold the
	 * lock (all waiters do so within the lock).
	 */

	if (bp->b_flags & (B_ERROR|B_INVAL)) {
		/*
		 * block has no info ... put at front of most free list,
		 * and arrange to release reference on b_vp.
		 * Also remove from current hash-bucket and hash to "self",
		 * so not clogging up a hash-bucket.
		 * NOTE: releasing B_INVAL buffer zaps b_vp ==> can't be
		 * found; thus no need to test B_INVAL in searches.
		 */
		bp->b_flags &= ~B_DELWRI;		/* cancel IO */
		if (bp->b_bufsize <= 0)
			flist = &bfreelist[BQ_EMPTY];
		else
			flist = &bfreelist[BQ_AGE];
		binsheadfree(bp, flist);
		bremhash(bp);
		bhashself(bp);
#ifdef	NFS
		oldvp = bp->b_vp;
#endif	NFS
		bp->b_vp = NULLVP;
	} else {
		/*
		 * block is useful ... if "aged" put at tail of "age" list,
		 * else tail of LRU list.
		 */
		ASSERT_DEBUG(bp->b_bufsize > 0, "brelse: size 0");
		if (bp->b_flags & B_AGE)
			flist = &bfreelist[BQ_AGE];
		else
			flist = &bfreelist[BQ_LRU];
		binstailfree(bp, flist);
	}

	bp->b_flags &= ~(B_ASYNC|B_AGE|B_NOCACHE);
	sema_count(&bp->b_alloc) = 1;			/* is now available */

	/*
	 * If someone waiting for a free header, wake 'em (all) up.
	 * The wakeups are done outside the lock, since as each process
	 * redispatches it will immediately go for the lock again.
	 *
	 * Processes can only queue after we exit the lock if someone
	 * has already picked up the buf we just freed; this is ok since
	 * the buf will eventually be freed again.
	 */

	v_lock(&buf_lists, s);
	if (blocked_sema(&buf_wait))
		vall_sema(&buf_wait);
	CVN_RELE(oldvp);
}

/*
 * baddr()
 *	Return buffer if still in memory, else return 0.
 *
 * Used in execve() to cancel argument IO if the exec() fails.
 *
 * Could optimize BG_NOMISS further (in getblk()) to avoid waiting for
 * buffer if IO already started.  If often wait in this manner,
 * should do this.
 */

struct buf *
baddr(vp, blkno, size)
	struct	vnode *vp;
	daddr_t	blkno;
	int	size;
{
	return (getblk(vp, blkno, size, BG_NOMISS));
}

/*
 * getblk()
 *	Assign a buffer for the given block.
 *
 * If the appropriate block is already associated, return it;
 * otherwise search for the oldest non-busy buffer and reassign it.
 *
 * Option argument is used to handle special cases if the block is
 * found or not found in the cache.  This is used in breada() and
 * baddr() to get appropriate effects (avoiding need for "incore").
 *
 * Always set B_FILIO before return in case someone alloc'd a buffer
 * and used it for some non-file-sys purpose.
 */

struct buf *
getblk(vp, blkno, size, option)
	struct	vnode *vp;
	daddr_t	blkno;
	int	size;
	int	option;
{
	register struct buf *bp;		/* buffer header */
	register struct buf *dp;		/* target hash list */
	register struct buf *fp;		/* free-list */
	spl_t	 s;
#ifdef	NFS
	struct vnode	*oldvp;
#endif	NFS

	/*
	 * Search the cache for the block.  If we hit, but
	 * the buffer is in use for i/o, then we wait until
	 * the i/o has completed.
	 *
	 * Since have buf_lists locked, there is no other process searching
	 * or otherwise messing with the hash/free lists.  Thus, don't need
	 * full sema protocol on the b_alloc sema (eg, can test and fuss with
	 * count without entering the gate).
	 *
	 * Note that if we wait on allocation of a buf-header, there is no
	 * need to loop when we get it (eg, brelse() won't put it on the
	 * free-list since we're waiting).
	 *
	 * We must do a newptes() on the buffer address space before return
	 * in the event another processor changed the mapping of the buffer
	 * since we (l.me) last saw it.
	 */

	ASSERT_DEBUG(vp != NULLVP, "getblk: nullvp");
	dp = BUFHASH(vp, blkno);
loop:
	s = p_lock(&buf_lists, SPLBUF);
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
		if (bp->b_blkno != blkno || bp->b_vp != vp)
			continue;
		/*
		 * Found it.  If not immediately available, wait for it;
		 * Else pull off free-list, and in either case use it.
		 * If waiting, brelse() gives it to us even if an error
		 * occurred to it.
		 *
		 * If don't want it from cache, return zero (breada).
		 */

		if (option & BG_NOHIT) {
			v_lock(&buf_lists, s);		/* let lists go */
			return (0);			/* don't want it */
		}

		if (!BAVAIL(bp)) {
			p_sema_v_lock(&bp->b_alloc, PRIBIO+1, &buf_lists, s);
			if (bp->b_flags & (B_INVAL|B_ERROR)) {
				brelse(bp);
				goto loop;
			}
		} else {
			notavail(bp);			/* "allocate" it */
			v_lock(&buf_lists, s);		/* let lists go */
		}

		bp->b_iotype = B_FILIO;
		if (brealloc(bp, size) == 0)
			goto loop;			/* nested continue! */
		newptes(bp->b_un.b_addr, bp->b_bcount);
		return (bp);
	}

	/*
	 * Didn't find it.  Get a new one (eg, steal a header from free-list).
	 * Lists still locked here.
	 * This is basically an in-line expansion of getnewbuf(), done
	 * here to avoid having to release buf_lists locking.
	 */

	if (option & BG_NOMISS) {		/* don't want if not in cache */
		v_lock(&buf_lists, s);		/* let lists go */
		return (0);			/* didn't find it */
	}

	for (fp = &bfreelist[BQ_AGE]; fp > bfreelist; fp--)
		if (fp->av_forw != fp)
			break;

	if (fp == bfreelist) {			/* no free blocks */
		p_sema_v_lock(&buf_wait, PRIBIO+1, &buf_lists, s);
		goto loop;
	}

	/*
	 * Got a free header.  Take it from free-list.
	 * Start delayed write if necessary, else it's ours.
	 */

	bp = fp->av_forw;
	notavail(bp);
	if (bp->b_flags & B_DELWRI) {
		v_lock(&buf_lists, s);
		bp->b_flags |= B_ASYNC;
		bwrite(bp);
		goto loop;
	}

	bremhash(bp);
	binshash(bp, dp);
	bp->b_dev = vp->v_rdev;
	bp->b_blkno = blkno;
#ifdef	NFS
	oldvp = bp->b_vp;
#endif	NFS
	bp->b_vp = vp;
	
	v_lock(&buf_lists, s);			/* finally! */

	CVN_HOLD(vp);
	CVN_RELE(oldvp);

 	if (bp->b_flags&B_NOTREF)
 		l.cnt.v_rawasted++;

	bp->b_flags = 0;
	bp->b_error = 0;
	bp->b_iotype = B_FILIO;
	bfree(bp);
	if (brealloc(bp, size) == 0)
		goto loop;
	BIODONE(bp) = 0;			/* not "done" */
	newptes(bp->b_un.b_addr, bp->b_bcount);
	return (bp);
}

/*
 * geteblk()
 *	Get an empty block, not assigned to any particular device
 *
 * Note that getnewbuf(1) removes buffer from its hashed list.
 * We will hash it on itself, since it needs to be hashed (for later when
 * it is reallocated), but we don't want to re-enter the locks to do this.
 */

struct buf *
geteblk(size)
	int	size;
{
	register struct buf *bp;

	for(;;) {
		bp = getnewbuf(1);
		bp->b_flags = B_INVAL;
		bfree(bp);
		bhashself(bp);
		CVN_RELE(bp->b_vp);
		bp->b_vp = NULLVP;
		bp->b_error = 0;
		if (brealloc(bp, size))
			break;
	}
	newptes(bp->b_un.b_addr, bp->b_bcount);
	return (bp);
}

/*
 * brealloc()
 *	Allocate space associated with a buffer.
 *
 * If can't get space, buffer is released
 */

brealloc(bp, size)
	register struct buf *bp;
	int	size;
{
	register struct buf *ep;
	struct buf	*dp;
	daddr_t	start;
	daddr_t	last;
	spl_t	s;

	/*
	 * First need to make sure that all overlaping previous I/O
	 * is dispatched with.
	 */

	if (size == bp->b_bcount)
		return (1);
	if (size < bp->b_bcount) { 
		if (bp->b_flags & B_DELWRI) {
			bwrite(bp);
			return (0);
		}
		return (allocbuf(bp, size));
	}
	BIODONE(bp) = 0;
	if (bp->b_vp == NULLVP)
		return (allocbuf(bp, size));

	/*
	 * Search cache for any buffers that overlap the one that we
	 * are trying to allocate. Overlapping buffers must be marked
	 * invalid, after being written out if they are dirty. (indicated
	 * by B_DELWRI) A disk block must be mapped by at most one buffer
	 * at any point in time. Care must be taken to avoid deadlocking
	 * when two buffer are trying to get the same set of disk blocks.
	 *
	 * If just using file-sys or just using IFBLK then can't deadlock
	 * on going for overlapped block, since higher level (eg,
	 * file-sys) insures can't have two processes going for same
	 * block at same time (eg, file-sys space alloc and IFBLK always
	 * uses 2K chunks).  Under unusual conditions can deadlock in
	 * allocbuf() (somehow wind up holding all the buffer-pages in
	 * processes asking for more pages at same time).
	 *
	 * Could deadlock if using file-sys and IFBLK concurrently, if
	 * happen to overlap blocks concurrently; thus, we use the
	 * B_REALLOC flag to sense deadlock, and back-out.  This should
	 * be extremely rare, but is nasty if it occurs.  B_REALLOC is
	 * only set within buf_lists lock and only by "owner" of a buffer;
	 * if one process is sleeping here and another is about to sleep
	 * waiting for 1st processes buffer, the 2nd process looses its
	 * buffer.  To avoid spinning under even more rare circumstances,
	 * we sleep on &lbolt after giving up buffer.
	 *
	 * We need to check for overlaps in case file-sys recently freed
	 * some frags/blocks which were quickly reallocated and new IO's
	 * started -- could have diff buf-headers with different blkno's
	 * encoded that cache same disk block.
	 *
	 * Note: this could probably be optimized by not exiting locks unless
	 * have to block, calling brelse() with lock asserted, etc, but expect
	 * this increases lock latency a bunch.  This implementation insists
	 * on making a complete pass thru the given hash-queue and not
	 * finding a match (berk only re-started if blocked).
	 */

	start = bp->b_blkno;
	last = start + btodb(size) - 1;
	dp = BUFHASH(bp->b_vp, bp->b_blkno);
loop:
	s = p_lock(&buf_lists, SPLBUF);
	bp->b_flags |= B_REALLOC;
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		if (ep == bp || ep->b_vp != bp->b_vp)
			continue;
		/* look for overlap */
		if (ep->b_bcount == 0 || ep->b_blkno > last ||
		    ep->b_blkno + btodb(ep->b_bcount) <= start)
			continue;
		/*
		 * Found an overlap.  Allocate the buffer, and cause
		 * its IO to be done.  The allocation mimics getblk().
		 */
		if (!BAVAIL(ep)) {
			/*
			 * If the buf we want is B_REALLOC, could deadlock
			 * if sleep (rare, and impossible unless using
			 * IFBLK and file-sys at same time).
			 */

			if (ep->b_flags & B_REALLOC) {
				bp->b_flags &= ~B_REALLOC;
				v_lock(&buf_lists, s);
				bp->b_flags |= B_INVAL;
				brelse(bp);
				p_sema(&lbolt, PRIBIO+1);
				return (0);
			}
			p_sema_v_lock(&ep->b_alloc, PRIBIO+1, &buf_lists, s);
			if (ep->b_flags & (B_INVAL|B_ERROR)) {
				brelse(ep);
				goto loop;
			}
		} else {
			notavail(ep);
			v_lock(&buf_lists, s);
		}
		if (ep->b_flags & B_DELWRI) {
			bwrite(ep);
			goto loop;
		}
		ep->b_flags |= B_INVAL;
		brelse(ep);
		goto loop;			/* since we exited lock */
	}
	bp->b_flags &= ~B_REALLOC;
	v_lock(&buf_lists, s);
	return (allocbuf(bp, size));
}

/*
 * getnewbuf()
 *	Find a buffer which is available for use.
 *
 * Select something from a free list.
 * Preference is to AGE list, then LRU list.
 *
 * Caller deals with b_iowait value.
 * Caller also deals with b_vp, releasing if appropriate.
 */

struct buf *
getnewbuf(rmhash)
	int	rmhash;				/* true ==> remove from hash */
{
	register struct	buf	*bp;
	register struct	buf	*dp;
	spl_t	s;

	for(;;) {
		s = p_lock(&buf_lists, SPLBUF);

		for (dp = &bfreelist[BQ_AGE]; dp > bfreelist; dp--)
			if (dp->av_forw != dp)
				break;

		if (dp == bfreelist) {			/* no free blocks */
			p_sema_v_lock(&buf_wait, PRIBIO+1, &buf_lists, s);
			continue;
		}

		bp = dp->av_forw;
		notavail(bp);
		if (bp->b_flags & B_DELWRI) {
			v_lock(&buf_lists, s);
			bp->b_flags |= B_ASYNC;
			bwrite(bp);
			continue;
		}
		if (rmhash)
			bremhash(bp);

		v_lock(&buf_lists, s);

 		if (bp->b_flags&B_NOTREF)
 			l.cnt.v_rawasted++;
 
		return (bp);
	}
}

/*
 * biowait()
 *	Wait for I/O completion on the buffer; return errors to the user.
 *
 * Returns with bp's b_iowait sema having a unit (eg, BIODONE(bp) == 1).
 */

biowait(bp)
	register struct buf *bp;
{
	if (BIODONE(bp) == 0) {
		p_sema(&bp->b_iowait, PRIBIO);
		BIODONE(bp) = 1;
	}
	if (u.u_error == 0)
		u.u_error = geterror(bp);
}

/*
 * biodone()
 *	Mark I/O complete on a buffer.
 *
 * If someone should be called, e.g. the pageout cleanup
 * daemon, do so.  Otherwise, wake up anyone waiting for it.
 *
 * Can be called by interrupt handler (eg, driver).
 */

biodone(bp)
	register struct buf *bp;
{
	ASSERT_DEBUG(BIODONE(bp) <= 0, "biodone: dup biodone");
	if (bp->b_flags & B_CALL) {
		bp->b_flags &= ~B_CALL;
		(*bp->b_iodone)(bp);
		return;
	}
	if (bp->b_flags & B_ASYNC) {
		BIODONE(bp) = 1;
		brelse(bp);
	} else
		v_sema(&bp->b_iowait);
}

/*
 * blkflush()
 *	Insure that no part of a specified block is in an incore buffer.
 *
 * Used in fsync() and paging to insure parts of a file are on disk.
 *
 * Called from syncip() and flush_vp(), called to insure block from locked
 * inode is on disk.  Thus, is sufficient to match on blkno and `size'
 * parameter is unneeded.  This only works due to higher-level
 * constraints (eg, inode usage), since the block of the inode is "owned"
 * by that inode and is in the cache *only* if blkno matches.  This also
 * means that we can return here if we find a match, since there will be
 * no other match.  This fails if there is concurrent use of the disk
 * partition by IFBLK special-file that references same blocks as owned
 * by inode; if you do this you deserve what you get!
 *
 * We keep the `size' parameter in case there is ever a need for more
 * general form of this.  Be careful to avoid "deadlock" of multiple
 * processes finding same block (interesting loop occurs here).
 */

/*ARGSUSED*/
blkflush(vp, blkno, size)
	struct	vnode *vp;
	register daddr_t blkno;
	long	size;
{
	register struct buf *ep;
	struct buf	*dp;
	spl_t	s;

	ASSERT_DEBUG(vp != NULLVP, "blkflush: null vp");
	dp = BUFHASH(vp, blkno);
	s = p_lock(&buf_lists, SPLBUF);
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		if (ep->b_vp != vp)
			continue;
		/* look for match */
		if (ep->b_bcount == 0 || ep->b_blkno != blkno)
			continue;
		/*
		 * Found one.  Allocate the buf and start IO if necessary.
		 * Allocation ala getblk().
		 */
		if (!BAVAIL(ep)) {
			p_sema_v_lock(&ep->b_alloc, PRIBIO+1, &buf_lists, s);
			if ((ep->b_flags & (B_INVAL|B_ERROR)) == 0
			&&  (ep->b_flags & B_DELWRI)) {
				bwrite(ep);		/* synchronous */
			} else
				brelse(ep);
			return;				/* return after match */
		} else if (ep->b_flags & B_DELWRI) {	/* available, dirty */
			notavail(ep);			/* "allocate" it */
			v_lock(&buf_lists, s);		/* let lists go */
			bwrite(ep);			/* synchronous */
			return;				/* return after match */
		} else					/* available, !dirty */
			break;				/* nothing to do */
	}
	v_lock(&buf_lists, s);
}

/*
 * blkinval()
 *	Invalidate any blocks that overlap given (blkno,size).
 *
 * Used in ZFOD mmap allocation to insure no matching blocks in cache when
 * set up ZFOD space.
 *
 * Note that only matches are due to space released via truncate; thus
 * is just fine to invalidate the buffers.
 *
 * Algorithm is a clone of brealloc().
 *
 * Could be done via:
 *	bp = getblk(dev, blkno, size, 0);
 *	bp->b_flags |= B_INVAL;
 *	brelse(bp);
 * since getblk() does brealloc() and arranges no overlap buffers exist,
 * but this implementation is much more efficient.
 */

blkinval(vp, blkno, size)
	struct	vnode *vp;
	register daddr_t blkno;
	long	size;
{
	register struct buf *ep;
	register daddr_t blknosz;
	struct buf	*dp;
	spl_t	s;

	ASSERT_DEBUG(vp != NULLVP, "blkinval: null vp");
	blknosz = blkno + btodb(size);
	dp = BUFHASH(vp, blkno);
loop:
	s = p_lock(&buf_lists, SPLBUF);
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		if (ep->b_vp != vp)
			continue;
		/* look for overlap */
		if (ep->b_bcount == 0 || ep->b_blkno >= blknosz ||
		    ep->b_blkno + btodb(ep->b_bcount) <= blkno)
			continue;
		/*
		 * Found one.  Invalidate it.
		 * BAVAIL() case could just set B_INVAL, but want brelse()
		 * to zap b_vp so (eg) getblk() doesn't need test B_INVAL.
		 */
		if (BAVAIL(ep)) {
			notavail(ep);
			v_lock(&buf_lists, s);
		} else {
			p_sema_v_lock(&ep->b_alloc, PRIBIO+1, &buf_lists, s);
		}
		ep->b_flags |= B_INVAL;
		brelse(ep);
		goto loop;
	}
	v_lock(&buf_lists, s);
}

/*
 * bflush()
 *	Make sure all write-behind blocks on dev (or NODEV for all)
 *	are flushed out (vp is passed not dev).
 *
 * Called from umount, sync, and close of a block device.
 *
 * Since all interesting headers for a given `vp' are in some hash-bucket,
 * algorithm loops on each hash-bucket until the hash-bucket no longer
 * contains relevant buf-headers, then moves on to next hash-bucket (much
 * more efficient than restarting at beginning of the cache each time).  This
 * is heuristic in that concurrent activity on the same `vp' can place
 * headers in hash-buckets already visited; these headers will not be
 * flushed.  This is consistent with the needs of most callers, since such
 * concurrent activity ==> there is another "open" of the relevant vnode.
 * Other callers insure no additional activity on the `vp' (eg, by holding vp
 * locked).  Also, all calls to bflush(NULLVP) are heuristic, and binval(vp)
 * stops if it finds a dirty buffer.
 *
 * Note that in TMP system, can have other processors placing buf's into the
 * free-list in the windows when we unlock buf_lists here.  This could
 * (conceiveably) result in this procedure never returning.  This "cannot"
 * happen, since that requires other processors to produce buffers at a rate
 * faster than this can start IO on them for a continued period of time
 * (extremely unlikely).  Even more unlikely since look only at one hash-bucket
 * at a time.  If necessary, this algorithm could be throttled by never making
 * more than 2*nbuf passes (or someting), but this risks not getting some
 * buffers.
 */

bflush(vp)
	register struct	vnode *vp;
{
	register struct buf *bp;
	register struct bufhd *hp;
	spl_t	s;

	(void)p_sema(&buf_invals, PZERO);
	for (hp = bufhash; hp < bufhashBUFHSZ; hp++) {
		if (hp->b_forw == (struct buf *) hp)	/* empty hash bucket */
			continue;
	loop:	s = p_lock(&buf_lists, SPLBUF);
		for (bp = hp->b_forw; bp != (struct buf *)hp; bp = bp->b_forw) {
			if ((vp == bp->b_vp || vp == NULLVP) &&
			    BAVAIL(bp) && (bp->b_flags & B_DELWRI)) {
				notavail(bp);		/* "allocate" */
				v_lock(&buf_lists, s);	/* avoid long latency */
				bp->b_flags |= B_ASYNC;	/* asynch IO */
				bwrite(bp);		/* start it */
				goto loop;
			}
		}
		v_lock(&buf_lists, s);
	}
	v_sema(&buf_invals);
}

/*
 * geterror()
 *	Pick up the device's error number and pass it to the user.
 *
 * If there is an error but the number is 0 set a generalized
 * code.
 */

geterror(bp)
	register struct buf *bp;
{
	int error = 0;

	if (bp->b_flags&B_ERROR)
		if ((error = bp->b_error) == 0)
			return (EIO);
	return (error);
}

/*
 * binval()
 *	Invalidate in core blocks belonging to closed or umounted filesystem.
 *
 * This is done via "allocating" each buffer associated with the device,
 * making it invalid, and brelse()'ng it.  This is done when closing a
 * device to insure no content from it in case it is soon reopened.
 * This should be preceeded by bflush() if keeping data is important,
 * as write-behind blocks don't get written.
 *
 * Any dirty buffers found ==> quit the algorithm.  Calling bflush()
 * before binval() insures this can only happen when the argument vnode
 * is concurrently in use and being written on; in this case it's
 * inappropriate to invalidate dirty buffers.  This can happen with two
 * or more opens of a block special file (due to every-open/every-close
 * semantics), or in NFS when trying to unmount a busy file-system.
 * When/if there's a need for a bio function to invalidate regardless of
 * "dirty" state, can add a "force" parameter here.
 *
 * The sempahore "buf_invals" has been added to serialise this. Although
 * this is somewhat clunky it does solve the problem and should not be
 * a high performance hit since closes of block devices is a low runner.
 * - more elegent solutions would just be that.
 *
 * Algorithm loops on each hash-bucket until the hash-bucket no longer
 * contains relevant buf-headers, then moves on to next hash-bucket (much
 * more efficient than restarting at beginning of the cache each time).
 * This is heuristic in that concurrent activity on the same `vp' can place
 * headers in hash-buckets already visited; these headers will not be
 * invalidated.  This is consistent with the needs of all callers, since
 * such concurrent activity ==> there is another "open" of the relevant
 * vnode, and the binval() is strictly speaking not necessary.
 *
 * The buffer must be "allocated" to be able to set the invalid bit;
 * this is purely a TMP concern, but is much cleaner than (eg) 4.2/4.3.
 *
 * This can cause bad latency on the buf_lists lock; fortunately, this
 * is done rarely.
 */

binval(vp)
	struct	vnode *vp;
{
	register struct buf *bp;
	register struct bufhd *hp;
	spl_t	s;

	ASSERT_DEBUG(vp != NULLVP, "binval: null vp");

	(void)p_sema(&buf_invals, PZERO);
	for (hp = bufhash; hp < bufhashBUFHSZ; hp++) {
	loop:	if (hp->b_forw == (struct buf *) hp)	/* empty hash bucket */
			continue;
		s = p_lock(&buf_lists, SPLBUF);
		for (bp = hp->b_forw; bp != (struct buf *)hp; bp = bp->b_forw) {
			if (bp->b_vp == vp) {
				/*
				 * Found one.  Invalidate it.
				 * BAVAIL() case could just set B_INVAL,
				 * but want brelse() to zap b_vp.
				 */
				if (BAVAIL(bp)) {
					notavail(bp);
					v_lock(&buf_lists, s);
				} else {
					p_sema_v_lock(&bp->b_alloc, PRIBIO,
								&buf_lists, s);
				}
				/*
				 * Dirty buffer ==> there is concurrent use.
				 */
				if (bp->b_flags & B_DELWRI) {
#ifdef	DEBUG
					printf("%s pid %d binval: dirty buffer\n",
						u.u_comm, u.u_procp->p_pid);
#endif	DEBUG
					brelse(bp);
					goto out;
				}
				bp->b_flags |= B_INVAL;
				brelse(bp);
				goto loop;
			}
		}
		v_lock(&buf_lists, s);
	}
out:
	v_sema(&buf_invals);
}

#ifdef	NFS
/*
 * See if the block is associated with some buffer
 * (mainly to avoid getting hung up on a wait in breada)
 *
 * This algorithm is heuristic in nature. If the caller has locked the
 * vp, then it can be assured that no entries will be made for the vp.
 * However note that the buffer found may be on its way to the free list, so
 * the caller is not guaranteed that the buffer will remain on the hash list.
 *
 * Currently incore() is only called from rwvp() in the nfs client code.
 */

incore(vp, blkno)
	struct vnode *vp;
	daddr_t blkno;
{
	register struct buf *bp;
	register struct buf *dp;
	spl_t s;

	dp = BUFHASH(vp, blkno);
	s = p_lock(&buf_lists, SPLBUF);
	for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
		if (bp->b_blkno == blkno && bp->b_vp == vp) {
			v_lock(&buf_lists, s);
			return (1);
		}
	v_lock(&buf_lists, s);
	return (0);
}

/*
 * Invalidate blocks associated with vp which are on the freelist.
 * Make sure all write-behind blocks associated with vp are flushed out.
 *
 * This is called from nfs_attrcache() to flush all buffers on the free list
 * for the particular vnode. The vnode is NOT locked. Thus, this may
 * race with concurrent readers/writers. However, it will not return until
 * it has successfully traversed the freelist without finding any buffers
 * associated with the vnode.
 */

binvalfree(vp)
	struct vnode *vp;
{
	register struct buf *bp;
	register struct buf *flist;
	int s;

	ASSERT_DEBUG(vp != NULLVP, "binvalfree: NULLVP");
loop:
	s = p_lock(&buf_lists, SPLBUF);
	for (flist = bfreelist; flist < &bfreelist[BQ_EMPTY]; flist++) {
		for (bp = flist->av_forw; bp != flist; bp = bp->av_forw) {
			if (vp == bp->b_vp) {
				if (bp->b_flags & B_DELWRI) {
					notavail(bp);
					v_lock(&buf_lists, s);
					bp->b_flags |= B_ASYNC;
					bwrite(bp);
				} else {
					notavail(bp);
					v_lock(&buf_lists, s);
					bp->b_flags |= B_INVAL;
					brelse(bp);
				}
				goto loop;	
			}
		}
	}
	v_lock(&buf_lists, s);
}
#endif	NFS

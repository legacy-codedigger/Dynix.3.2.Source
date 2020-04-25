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
static	char	rcsid[] = "$Header: vm_swp.c 2.16 91/02/12 $";
#endif

/*
 * vm_swp.c
 *	Various kinds of physical IO: physio(), swap(), etc.
 */

/* $Log:	vm_swp.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/trace.h"
#include "../h/map.h"
#include "../h/uio.h"
#include "../h/cmn_err.h"

#include "../balance/engine.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"
#include "../machine/gate.h"

/*
 * Swap IO headers -
 * They contain the necessary information for the swap I/O.
 * At any given time, a swap header can be in three
 * different lists. When free it is in the free list, 
 * when allocated and the I/O queued, it is on the swap 
 * device list, and finally, if the operation was a dirty 
 * page push, when the I/O completes, it is inserted 
 * in a list of cleaned pages to be processed by the cleanup daemon.
 */

struct	buf	*swbuf;			/* set in allockds() to alloc'd hdrs */
struct	buf	*bswlist;		/* free-list header */
lock_t		swbuf_mutex;		/* lock alloc/dealloc of swap header */
sema_t		swbuf_wait;		/* wait here for swap headers */

struct	buf	*swbuf_alloc();

#ifdef	PERFSTAT
int		numswfree;		/* current # free swap buffers */
int		minswfree;		/* min # free swap headers ever */
#endif	PERFSTAT

/*
 * bswinit()
 *	Count swap devices and initialize linked list of free swap headers.
 *
 * These do not actually point to buffers, but rather to pages that
 * are being swapped in and out.
 */

bswinit()
{
	register struct buf *sp;
	register int i;
	register int gateno;
	struct swdevt *swp;
	extern	caddr_t	topmem;

	/*
	 * Count swap devices, and adjust total swap space available.
	 * Some of this space will not be available until a vswapon()
	 * system is issued, usually when the system goes multi-user.
	 */

	nswdev = 0;
	nswap = 0;
	for (swp = swdevt; swp->sw_dev; swp++) {
		nswdev++;
		if (swp->sw_nblks > nswap)
			nswap = swp->sw_nblks;
	}
	if (nswdev == 0) {
		panic("binit: no swap");
		/*
		 *+ No swap device have been configured. Check the
		 *+ system configuration files.
		 */
	}
	if (nswdev > 1)
		nswap = ((nswap + dmmax_sw - 1) / dmmax_sw) * dmmax_sw;
	else if ((int)topmem >= LOTSOFMEM*1024*1024 || Nengine >= LOTSOFCPUS)
		CPRINTF("WARNING: should run interleaved swap with >= %dMb or >= %d processors.\n",
		    LOTSOFMEM, LOTSOFCPUS);
	nswap *= nswdev;
	maxpgio *= nswdev;
	(void) swfree(0);

	/*
	 * Init swap-header alloc/wait list and mutex structures,
	 * and swbuf header semas.
	 *
	 * Bufinit() does a redundant init of "b_alloc" semaphore,
	 * since allocation is off the swap-header list (bswlist).
	 *
	 * bswlist is initially NULL, since it lives in .bss
	 */

	init_lock(&swbuf_mutex, G_SWLIST);
	init_sema(&swbuf_wait, 0, 0, G_SWWAIT);

	gateno = 0;
	for (i = 0, sp = swbuf; i < nswbuf; i++, sp++) {
		bufinit(sp, (gate_t)(G_SWMIN + gateno++ % (G_SWMAX-G_SWMIN+1)));
		sp->av_forw = bswlist;
		bswlist = sp;
	}

	init_sema(&drain_dirty, 0, 0, G_PAGEOUT);
	init_lock(&swpq_mutex, G_PAGEOUT);
	init_sema(&swapout_sync, 0, 0, G_PAGEOUT);
	init_sema(&runout, 0, 0, G_PAGEOUT);
}

/*
 * swap I/O -
 *	Copy some pages in/out of memory.
 *
 * 'pte' arg is pte of 1st page involved with the IO; pte's assumed
 * contiguous.
 *
 * This is basically a front-end to swapio(), to allow pageout() to
 * conditionally allocate swap-buf headers.
 */

swap(p, dblkno, pte, nbytes, flag, vp)
	struct	proc	*p;
	swblk_t		dblkno;
	struct	pte	*pte;
	int		nbytes;
	int		flag;
	struct vnode	*vp;
{
	struct buf	*bp;

	/*
	 * Allocate a swap-header and set it up for the IO,
	 * waiting for it if none immediately available, then
	 * do the swapio and release the buffer.
	 */

	bp = swbuf_alloc(1);
	swapio(bp, p, dblkno, pte, nbytes, flag, vp);
	swbuf_free(bp);
}

/*
 * swapio()
 *	Swap-IO -- also allows asynch IO to be started if B_DIRTY is
 *	in `flag'.
 *
 * 'pte' arg is pte of 1st page involved with the IO; pte's assumed
 * contiguous.
 *
 * Assumes all drivers that get swap/page IO can handle a KLMAX*CLBYTES
 * size request for either B_PTEIO or B_PTBIO.
 */

swapio(bp, p, dblkno, pte, nbytes, flag, vp)
	register struct buf *bp;
	struct	proc	*p;
	swblk_t		dblkno;
	struct	pte	*pte;
	int		nbytes;
	register int	flag;
	struct vnode	*vp;
{
	register u_int	c;

	/*
	 * Set up buf-header for the IO.
	 */

	bp->b_flags = B_PHYS | flag;
	if ((flag & (B_DIRTY|B_PGIN)) == 0)
		if (flag & B_READ)
			l.cnt.v_pswpin += btoc(nbytes);
		else
			l.cnt.v_pswpout += btoc(nbytes);
	bp->b_proc = p;				/* information only */
	bp->b_un.b_pte = pte;
	bp->b_iotype = (flag & B_PAGET) ? B_PTBIO : B_PTEIO;
	while (nbytes > 0) {
		bp->b_bcount = nbytes;
		bp->b_blkno = dblkno;
		bp->b_dev = vp->v_rdev;
		bp->b_vp = vp;
		if (nbytes > (KLMAX-1)*CLBYTES) {
			/*
			 * Big enough that must let driver limit it.
			 * Insure transfer is multiple of CLBYTES in
			 * case B_PTEIO and driver limits to non-multiple.
			 */
			VOP_MINPHYS(bp);
			bp->b_bcount &= ~CLOFSET;
		}
		c = bp->b_bcount;
		ASSERT((c & (NBPG-1)) == 0, "swapio: count");
		/*
		 *+ The swapio function, which does the actual swapping
		 *+ of pages, was invoked with a size that wasn't a multiple
		 *+ of the system page size.
		 */
		physstrat(bp, vp->v_op->vn_strategy, PSWP);
		if (flag & B_DIRTY) {
			ASSERT(c == nbytes, "swapio: big push");
			/*
			 *+ swapio initiated an asynchronous write with
			 *+ a size that was greater than the maximum size
			 *+ that drivers can handle in a single request.
			 */
			return;
		}
		bp->b_un.b_pte += btop(c);
		if (bp->b_flags & B_ERROR) {
			/*
			 * Currently, panic if can't Read or Write a
			 * page-table or Uarea (this can be handled better).
			 * If reading or writing normal pages, can just kill.
			 */
			if (flag & (B_UAREA|B_PAGET))
				panic("swapio: hard IO err in swap");
				/*
				 *+ A hard disk I/O error occurred when the
				 *+ kernel tried to swap a process's page
				 *+ tables or U-area.
				 */
			/*
			 * If vp is marked bad, assume mapped file error
			 * else generic swap IO error.
			 */
			if (vp->v_badmap)
				mmap_swkill(p, (int) vp->v_badmap);
			else
				swkill(p, "swap IO error");
			break;
		}
		nbytes -= c;
		dblkno += btodb(c);
	}
}

/*
 * swbuf_alloc()
 *	Allocate a swap-buf header.  Conditionally wait for one if none free.
 */

struct buf *
swbuf_alloc(can_wait)
	int	can_wait;
{
	struct buf *bp;
	spl_t	s;

	for (;;) {
		s = p_lock(&swbuf_mutex, SPLSWP);
		if (bswlist) {
			bp = bswlist;
			bswlist = bp->av_forw;
#ifdef	PERFSTAT
			if (--numswfree < minswfree)
				minswfree = numswfree;
#endif	PERFSTAT
			v_lock(&swbuf_mutex, s);
			return (bp);
		} else if (!can_wait) {
			v_lock(&swbuf_mutex, s);
			return (0);
		}
		p_sema_v_lock(&swbuf_wait, PSWP+1, &swbuf_mutex, s);
	}
}

/*
 * swbuf_free()
 *	Release a swap-buf header.
 */

swbuf_free(bp)
	register struct buf *bp;
{
	spl_t	s;

	s = p_lock(&swbuf_mutex, SPLSWP);
	bp->b_flags &= ~(B_PHYS|B_PAGET|B_UAREA|B_DIRTY);
	bp->av_forw = bswlist;
	bswlist = bp;
	if (blocked_sema(&swbuf_wait))
		vall_sema(&swbuf_wait);
#ifdef	PERFSTAT
	++numswfree;
#endif	PERFSTAT
	v_lock(&swbuf_mutex, s);
}

/*
 * mmap_swkill()
 *	Kill a process due to mapped-file fault error.
 */

mmap_swkill(p, reason)
	struct	proc	*p;
	int	reason;
{
	swkill(p, (reason & MM_STALETEXT)
			? "text modification"
			: "IO error in program text or mapped file");
}

/*
 * swkill()
 *	Print mesg as to why, then kill process.
 */

swkill(p, mesg)
	struct proc *p;
	char	*mesg;
{

	/*
	 * To be sure no looping (e.g. sched() trying to swap out) mark
	 * process locked in core (as though done by user) so no one will
	 * try to swap it out.
	 */

	++p->p_noswap;

	/*
	 * If process has already been killed, do nothing.
	 */

	if (p->p_sig & sigmask(SIGKILL))
		return;

	/*
	 * Generate a "useful" message and kill the process.
	 */

	printf("pid %d: killed due to %s\n", p->p_pid, mesg);
	/*
	 *+ A process was killed during a swap operation due to
	 *+ an error or a resource shortage.  The message printed
	 *+ will give more information.
	 */
	uprintf("sorry, pid %d was killed due to %s\n", p->p_pid, mesg);
        /*
         *+ A process was killed during a swap operation due to
         *+ an error or a resource shortage.  The message printed
         *+ will give more information.
         */

	psignal(p, SIGKILL);
}

/*
 * physio()
 *	Raw I/O.  Called from driver read/write routines.
 *
 * The arguments are
 *	The strategy routine for the device
 *	A buffer, which will always be a special buffer
 *	  header owned exclusively by the device for this purpose
 *	The device number
 *	Read/write flag
 *	device-specific transfer count limiting procedure (mincnt)
 *
 * Essentially all the work is computing physical addresses and
 * validating them.  If the user has the proper access privileges, the
 * process is marked not swappable and the pages involved in the I/O are
 * faulted and "locked".  The locking is based on the local model VM,
 * implicitly locking by faulting into the resident set (see vslock()).
 *
 * This allocates the buffer again for each element of the iovec.  Could
 * be smarter and allocate once, but think this isn't worth much.
 *
 * This physio() works for mono-P drivers if they have same buf-header
 * and have initialized the buffers (see bufinit()).
 */

/*
 * Max size RAW IO request (bytes).
 */

long	max_RAW_IO = 128*1024;

physio(strat, bp, dev, rw, mincnt, uio)
	int		(*strat)(); 
	register struct buf *bp;
	dev_t		dev;
	int		rw;
	int		(*mincnt)(); 
	struct	uio	*uio;
{
	register struct iovec *iov;
	register int	c;
	register struct	proc *p = u.u_procp;
	caddr_t		a;
	int		resid;
	int		error = 0;
	long		rs_need;
	int		abflg;

	if (abflg = (bp == NULL))
		bp = swbuf_alloc(1);

	for (iov = uio->uio_iov; uio->uio_iovcnt; iov++, uio->uio_iovcnt--) {
		/*
		 * Verify address space can accept the transfer.
		 * Note: useracc() fails phys-mapped address space.
		 */
		if (useracc(iov->iov_base,(u_int)iov->iov_len,rw==B_READ?B_WRITE:B_READ) == NULL) {
			error = EFAULT;
			break;
		}

		p_sema(&bp->b_alloc, PRIBIO+1);		/* allocate buffer */
		bp->b_error = 0;
		bp->b_flags = B_PHYS;			/* iov_len == 0? */
		bp->b_proc = p;
		bp->b_un.b_addr = iov->iov_base;

		while (iov->iov_len > 0) {
			bp->b_flags = B_PHYS | rw;
			bp->b_dev = dev;
			bp->b_blkno = btodb(uio->uio_offset);
			bp->b_bcount = iov->iov_len;

			/*
			 * Restrict transfer to what driver/HW can manage,
			 * and in any case to a max system IO size.
			 * Then determine exact Rset size needed for the
			 * transfer and grow Rset (if necessary) to allow it
			 * (the idea being to go for greater device thruput).
			 *
			 * PFF won't adjust Rset during this since
			 * process won't do PFF adjust unless returning
			 * to user-mode.
			 */

			bp->b_iotype = B_RAWIO;
			if (bp->b_bcount > max_RAW_IO)
				bp->b_bcount = max_RAW_IO;
			(*mincnt)(bp);

			c = bp->b_bcount;
			a = bp->b_un.b_addr;

			rs_need = (((int)a & CLOFSET) + c + CLOFSET) / CLBYTES;
			if (p->p_rscurr < rs_need) {
				/*
				 * If can't adjust, fail the IO -- system
				 * has unreasonable vt_maxRS and truncating
				 * IO can do strange things (eg, to tape).
				 */
				vsetRS(rs_need);
				if (p->p_rscurr < rs_need) {
					bp->b_flags |= B_ERROR;
					bp->b_error = ENOMEM;
					break;
				}
#ifdef	DEBUG
				printf("<%d>: RAW grow Rset to %d\n",
					u.u_procp-proc, rs_need);
#endif	DEBUG
			}

			/*
			 * Nail the pages in memory and do the transfer.
			 * Note: current vslock() allows bump p_noswap
			 * after vslock() returns -- maybe better to
			 * have vslock() bump, vsunlock() decr.
			 */

			++p->p_noswap;			/* non-swappable */
			vslock(a, c, rw & B_READ);
			if (rw & B_READ)
				l.cnt.v_phyrc++;
			else
				l.cnt.v_phywc++;
			physstrat(bp, strat, PRIBIO);
			/* vsunlock(a, c, rw); */	/* local VM no need */
			--p->p_noswap;			/* re-swappable */
			c -= bp->b_resid;
			if (rw & B_READ) 
				l.cnt.v_phyr += btodb(c);
			else
				l.cnt.v_phyw += btodb(c);
			bp->b_un.b_addr += c;
			iov->iov_len -= c;
			uio->uio_resid -= c;
			uio->uio_offset += c;
			/* temp kludge for tape drives */
			if (bp->b_resid || (bp->b_flags&B_ERROR))
				break;
		}
		bp->b_flags &= ~B_PHYS;
		error = geterror(bp);
		resid = bp->b_resid;
		v_sema(&bp->b_alloc);			/* release buffer */
		if (resid || error)			/* resid: for tapes */
			break;
	}
	if (abflg)
		swbuf_free(bp);
	return (error);
}

/*
 * minphys()
 *	Default procedure to limit phys, swap transfers.
 *
 * Most drivers will have their own procedure.
 */

unsigned
minphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > max_RAW_IO)
		bp->b_bcount = max_RAW_IO;
}

/*
 * Called from driver read/write routines before physio. physck
 * truncates raw i/o requests that overflow partition boundaries.
 *
 * returns 0 if ok (with possible changes in the request size).
 * returns -1 if reading from exactly the end of the partition
 *	ie ok, but i/o not required since size is 0.
 * returns EINVAL otherwise.
 */
physck(lim, uio, rw, diff)
	off_t lim;
	struct uio *uio;
	int *diff;
{
	register int cnt;
	register struct iovec *iov;
	register off_t hi;

	*diff = 0;
	if (uio->uio_offset >= lim) {
		if (uio->uio_offset > lim || rw == B_WRITE)
			return(EINVAL);
		return(-1);	/* ok, but no i/o */
	}
	hi = uio->uio_offset + uio->uio_resid;
	if (hi > lim) {
		cnt = hi - lim;
		*diff = cnt;	/* return diff between requested and actual */
		uio->uio_resid -= cnt;
		iov = uio->uio_iov; iov += uio->uio_iovcnt;
		while (cnt) {
			--iov;
			if (iov->iov_len <= cnt) {
				if (--uio->uio_iovcnt == 0)
					return(EINVAL);
				cnt -= iov->iov_len;
			} else {
				iov->iov_len -= cnt;
				cnt = 0;
			}
		}
	}
	return(0);
}

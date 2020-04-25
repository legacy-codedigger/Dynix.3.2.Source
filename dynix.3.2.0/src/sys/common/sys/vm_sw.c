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
static	char	rcsid[] = "$Header: vm_sw.c 2.13 1991/05/23 17:15:15 $";
#endif

/*
 * vm_sw.c
 *	Swap "driver"; handles interleaving.
 */

/* $Log: vm_sw.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/vnode.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/uio.h"
#include "../h/file.h"
#include "../h/vmmeter.h"

#include "../machine/vmparam.h"
#include "../machine/pte.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"

int	nswdev;			/* number of swap devices */
int	nswap;			/* size of swap space */

int	nswapmap;		/* # entries in swap-space map */
struct	map	*swapmap;	/* swap-space allocation map */

struct	map	*argmap;	/* argument "file" space map */

int	max_swapmap;		/* most free space ever */
int	min_swapmap=0x7fffffff;	/* least free space ever */
int	cur_swapmap;		/* current free space */

/*
 * swstrat()
 *	Indirect driver for multi-controller paging.
 *
 * Note that since physio() doesn't know about interleaved swap, RAW
 * IO only tracks real swap-space if requests don't cross dmmax_sw boundaries.
 * Swap IO obeys this.
 */

#define	MINIROOTSIZE	6420
daddr_t end_of_root = MINIROOTSIZE;

swstrat(bp)
	register struct buf *bp;
{
	register struct swdevt *sw;
	int	sz;
	int	off;
	int	seg;
	extern	dev_t rootdev, argdev;
#ifdef DEBUG
	static int	once=0;
#endif

	/*
	 * A mini-root gets copied into the front of the swap
	 * and we run over top of the swap area just long
	 * enough for us to do a mkfs and restor of the real
	 * root (sure beats rewriting standalone restor).
	 */
	if (rootdev == argdev) {
#ifdef DEBUG
		if (once == 0 ) {
			printf("swap offset by %d for mini-root\n", 
								end_of_root);
			once = 1;
		}
#endif
		bp->b_blkno += end_of_root;
	}

	sz = howmany(bp->b_bcount, DEV_BSIZE);
	if (nswdev > 1) {
		/*
		 * Insist request doesn't cross dmmax_sw boundary.
		 */
		off = bp->b_blkno % dmmax_sw;
		if (off+sz > dmmax_sw) {
			bp->b_flags |= B_ERROR;
			iodone(bp);
			return;
		}
		seg = bp->b_blkno / dmmax_sw;
		sw = &swdevt[seg % nswdev];
		bp->b_blkno = (seg / nswdev) * dmmax_sw + off;
	} else
		sw = &swdevt[0];

	ASSERT(sw->sw_dev != 0, "swstrat");
	/*
	 *+ The swap device driver has been accesed but no disk has been
	 *+ attatched to the swap device.
	 */
	bp->b_dev = sw->sw_dev;

	/*
	 * Insure it fits in swap partition.  Note that this makes ref
	 * to non-existent swap-partition illegal (sw_nblks == -1).
	 */

	if (!sw->sw_freed || bp->b_blkno+sz > sw->sw_nblks) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
	} else
		(*bdevsw[major(sw->sw_dev)].d_strategy)(bp);
}

/*
 * swminphys()
 *	Indirect minphys procedure -- call the one from the real driver.
 *
 * Don't call driver minphys() if out of range on blocks; swstrat() will
 * error the request.
 *
 * Check is redundant with swstrat(); done here in case called thru
 * swread()/swwrite().  Awkward in swread()/swwrite() since uio may
 * reference multiple swap chunks.  Most page IO's won't call swminphys()
 * anyhow (see swapio()).
 *
 * Note: very low probability race with swapon() turning on sw_freed
 * between now and physio() calling swstrat() (won't have limited count).
 */

swminphys(bp)
	register struct buf *bp;
{
	register struct swdevt *sw;
	int	seg;
	daddr_t blkno;

	if (nswdev > 1) {
		seg = bp->b_blkno / dmmax_sw;
		sw = &swdevt[seg % nswdev];
		blkno = (seg / nswdev) * dmmax_sw + (bp->b_blkno % dmmax_sw);
	} else {
		sw = &swdevt[0];
		blkno = bp->b_blkno;
	}

	if (sw->sw_freed
	&&  (blkno + howmany(bp->b_bcount, DEV_BSIZE)) <= sw->sw_nblks) {
		bp->b_dev = sw->sw_dev;
		(*bdevsw[major(bp->b_dev)].d_minphys)(bp);
	}
}

swread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (physio(swstrat, (struct buf *)0, dev, B_READ, swminphys, uio));
}

swwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	return (physio(swstrat, (struct buf *)0, dev, B_WRITE, swminphys, uio));
}

/*
 * swapon()
 *	System call swapon(name) enables swapping on device name,
 *	which must be in the swdevsw.
 *
 * Return EBUSY if already swapping on this device.
 *
 * Note that swapconf() has taken care of making unusable any swap
 * partitions that have bad size (typically, not-present device).
 */

swapon()
{
	struct a {
		char	*name;
	} *uap;
	struct vnode *vp;
	register struct swdevt *sp;
	dev_t	dev;
	GATESPL(s);

	uap = (struct a *)u.u_ap;
	u.u_error =
		lookupname(uap->name, UIOSEG_USER, FOLLOW_LINK,
			(struct vnode **)0, &vp);
	if (u.u_error)
		return;
	if (vp->v_type != VBLK) {
		u.u_error = ENOTBLK;
		VN_PUT(vp);
		return;
	}
	dev = (dev_t)vp->v_rdev;
	VN_PUT(vp);
	if (major(dev) >= nblkdev) {
		u.u_error = ENXIO;
		return;
	}

	/*
	 * Search starting at first table entry, even though
	 * first (primary swap area) is freed at boot, to get
	 * EBUSY instead of ENODEV if try to re-swapon it (less
	 * confusing error).
	 *
	 * We redundantly set sw_freed (swfree will also), but this
	 * avoids deadlock in swfree's call to swapfree() while
	 * avoiding holding G_SWAP across entire call to swfree (and
	 * hence to driver open procedure).  Swfree sets sw_freed
	 * for swfree(0) call in main.
	 */

	for (sp = &swdevt[0]; sp->sw_dev; sp++) {
		if (sp->sw_dev == dev) {
			P_GATE(G_SWAP, s);
			if (sp->sw_freed) {
				u.u_error = EBUSY;
				V_GATE(G_SWAP, s);
				return;
			}
			V_GATE(G_SWAP, s);
			u.u_error = swfree(sp - swdevt);
			return;
		}
	}
	u.u_error = ENODEV;
}

/*
 * swfree()
 *	Free the index'th portion of the swap map.
 *
 * Each of the nswdev devices provides 1/nswdev'th of the swap
 * space, which is laid out with blocks of dmmax_sw pages circularly
 * among the devices.
 */

int
swfree(index)
	int index;
{
	register swblk_t vsbase;
	register long blk;
	dev_t dev;
	register swblk_t dvbase;
	register int nblks;
	int	err;
	int	mode;
	GATESPL(s);

	dev = swdevt[index].sw_dev;
	if (index == 0 && dev == rootdev) {
		mode = FREAD|FWRITE|FSPEC; 	/* mini-root */
	} else {
		mode = FREAD|FWRITE|FSWAP;	/* normal */
	}
	if ((err = (*bdevsw[major(dev)].d_open)(dev, mode)) != 0) {
		return (err);
	}

	swdevt[index].sw_freed = 1;
	nblks = swdevt[index].sw_nblks;

	P_GATE(G_SWAP, s);
	min_swapmap += nblks;
	V_GATE(G_SWAP, s);

	for (dvbase = 0; dvbase < nblks; dvbase += dmmax_sw) {
		blk = nblks - dvbase;
		vsbase = index*dmmax_sw + dvbase*nswdev;
		ASSERT(vsbase < nswap, "swfree: too much swap space");
		/*
		 *+ The kernel discovered an inconsistency when adding a new 
		 *+ swap device.  It appeared that swap addresses greater 
		 *+ than the expected maximum were being added.
		 */
		if (blk > dmmax_sw)
			blk = dmmax_sw;
		if (vsbase == 0) {
			/*
			 * Can't free a block starting at 0 in the swapmap
			 * but need some space for argmap so use 1/2 this
			 * hunk which needs special treatment anyways.
			 */
			arginit(swdevt[0].sw_dev,
				(long)(blk/2-ctod(CLSIZE)), (long)ctod(CLSIZE));
			/*
			 * First of all chunks... initialize the swapmap
			 * to contain the second half of the hunk.
			 */
			rminit(swapmap, (long)blk/2, (long)blk/2, "swap", nswapmap);
			cur_swapmap = blk/2;
		} else {
			swapfree(blk, vsbase);
		}
	}
	return (0);
}

/*
 * swapalloc()
 *	Try to allocate space from the swap-map.
 */

swapalloc(size)
	long	size;
{
	long	blk;
	GATESPL(s);

	P_GATE(G_SWAP, s);
	blk = rmalloc(swapmap, size);
	if (blk != 0) {
		if ((cur_swapmap -= size) < min_swapmap)
			min_swapmap = cur_swapmap;
	}
	V_GATE(G_SWAP, s);

	return(blk);
}

/*
 * swapfree()
 *	Release space into the swap-map.
 */

swapfree(size, blk)
	long	size;
	long	blk;
{
	GATESPL(s);

	P_GATE(G_SWAP, s);
	rmfree(swapmap, size, blk);
	if ((cur_swapmap += size) > max_swapmap)
		max_swapmap = cur_swapmap;
	V_GATE(G_SWAP, s);
}

#if	defined(DEBUG) && defined(PERFSTAT)
/*
 * swapmap_stat()
 *	Report swapmap space stats.
 */

swapmap_stat()
{
	register struct mapent *ep;
	register int max_chunk;
	register int count;
	spl_t	s;

	count = max_chunk = 0;
	P_GATE(G_SWAP, s);

	for (ep = (struct mapent *)(swapmap+1); ep->m_size; ep++, count++) {
		if (ep->m_size > max_chunk)
			max_chunk = ep->m_size;
	}

	V_GATE(G_SWAP, s);

	printf("Swapmap: %d chunks, biggest=%d; free=%d, max=%d, min=%d\n",
			count, max_chunk, cur_swapmap, max_swapmap, min_swapmap);
}
#endif	DEBUG&PERFSTAT

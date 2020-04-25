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

#ifndef	lint
static	char	rcsid[] = "$Header: ufs_bmapcache.c 1.4 90/05/13 $";
#endif

/*
 * ufs_bmapcache.c
 *	Manage cache of UFS bmap translations.
 *
 * Useful for DBMS files and other long open large files that are frequently
 * accessed.  Avoids bmap() translation, which requires 2 buf-cache
 * transactions in double-indirect blocks (after approx 16Meg on 8k block
 * file-system).
 *
 * Semantics:
 *	Supports concurrent callers (eg, concurrent readers via rwsema_t's).
 *
 *	Coherent with all writes and truncates.
 *
 *	Hooks to allow memory reclamation.  Bmap cache can be deleted any time
 *	hold inode exclusively locked.  Will be automatically reinstantiated,
 *	almost certainly at different address.  This implies TLB fussing.
 *
 *	Direct blocks are bmap'd the standard way -- no need to "cache"
 *	these, since inode already holds them.
 *
 *	Caching currently disabled when inode goes inactive (see TODO).
 *
 *	Currently, use SysBmapCache() system call to turn this on.
 *
 *	Will (attempt to) dynamically grow cache when writes to file occur.
 *	This can cause the allocated memory to move.  More TLB fussing.
 *
 *	TLB coherence is forced by FlushTLB() from balance/tlb.c.  Done at
 *	memory allocation time since expect the allocations to be "rare"
 *	w.r.t. usage, and this avoids need to check in SW on each BmapMap()
 *	call.  There can be times where the bmap cache is frequently grown,
 *	resulting in many TLB-flushes, but typically this should settle down
 *	quickly.
 *
 * NOTES:
 *	Uses inode to provide locking for access.  All callers will hold
 *	inode locked exclusively or shared.  Holding inode exclusively
 *	allows arbitrary manipulation of the map.
 *
 *	Thus can have concurrent accessors.  Need to resolve race when
 *	multiple concurrent translations occur and map has been deleted.  Use
 *	local lock (bm_mutex) for this.  bm_mutex is only used in this case.
 *
 *	Memory allocation uses wmemall() and is careful about TLB flushing.
 *	Implementation is intimate with wmemall(); if wmemall() changes so
 *	must this module.  See BmapAllocMem(), BmapGrow().
 *
 * TODO:
 *	Add l.rablock, l.rasize semantics?
 *
 *	Create a header file (or add to existing one) for SysBmapCache "flavor"
 *	mnemonics (BMC_OFF, BMC_ON, BMC_QUERY).
 *
 *	Provide some mechanism to "auto-turn-on" bmap caching?  Eg, some
 *	combination of file mode bits, or a "permanent flag" bit.  This would
 *	allow the feature to be turned on without modifying programs.
 *
 *	BmapPurge() implementation assumes when inode goes inactive the bmap
 *	cache is purged; this avoids problematic synchronization with iget()
 *	on IFREE inodes.  When/if need to do this better (eg, if it's
 *	frequent for inode to go inactive then active and still want cache),
 *	a form of "conditional iget" could be implemented to solve this.
 *	Offhand, don't like throwing away information worked so hard for, but
 *	don't want to create ciget() for this prototype; should consider for
 *	true product.  ciget() should be in ufs/ufs_inode.c to keep
 *	abstractions in right places ;-)
 *
 *	BmapTrunc() currently just calls BmapFlush().  This is heavy handed,
 *	expects truncates (to non-zero offset) to be rare.  When/if this is a
 *	problem, BmapTrunc() is easily extended to zap relevant portion of
 *	the cache and possibly release some memory.  Might also use bmap
 *	cache during truncate operation to obtain block numbers easier.
 *
 *	Eventually should have "kernel pagable objects" and build this from
 *	such support.  Note that bmap cache need not have other backing
 *	store; rather it can re-instantiate pages from indirect blocks.
 *	Also need to accommodate "natural" page size of the object; eg, 8k
 *	for 8k block file-system.  Consider this a special prototype ;-)
 *
 *	Could get fancier and keep 2-level structure, allocating each block
 *	of indirect block cache separately.  Probably hold 1st level of 2nd
 *	level indirect in memory.  Would allow finer grain allocation and less
 *	fragmentation of kernel maps.  This is even closer to emulating a
 *	kernel paged object.  Initial implementation is simpler, but can
 *	fragment Usrptmap[] further.
 */

/* $Log:	ufs_bmapcache.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/kernel.h"
#include "../h/cmap.h"
#include "../h/llist.h"

#include "../ufs/fs.h"
#include "../ufs/inode.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/mftpr.h"
#include "../machine/intctl.h"
#include "../machine/gate.h"

/*
 * Struct BmapCache holds relevent data implementing the bmap cache for a
 * given inode.
 *
 * bm_daddr == NULL ==> no memory currently allocated.  Dynamically allocated
 * to reflect file size in BmapMap().  Simple linear array in kernel virtual
 * memory.
 *
 * bm_size reflects # blocks that can be mapped by memory held.  Only valid
 * when bm_daddr != NULL.
 *
 * Direct blocks are not cached in this memory.  Avoids need to worry about
 * extending frags.
 */

struct	BmapCache {
	dlink_t		bm_link;	/* list inclusion (must be first) */
	struct	inode	*bm_ip;		/* inode attached to */
	lock_t		bm_mutex;	/* lock installation of bm_daddr */
	daddr_t		*bm_daddr;	/* virtual address this lives at */
	int		bm_size;	/* size (# blocks) allocated to map */
};

#define	BMAP_LOCK(bm, s)	(s) = p_lock(&(bm)->bm_mutex, SPLIMP)
#define	BMAP_UNLOCK(bm, s)	v_lock(&(bm)->bm_mutex, (s))

/*
 * Module local definitions.
 *
 * BMAP_MAXBLOCK limits size of file to cache bmap information for.  This could
 * easily be made configurable.  Currently, set to max of 2Gig file on an 8K
 * blocked file-system (==> 1G on a 4k block file system).
 */

extern int		NumBmapCache;	/* set in conf/param.c */

static	lock_t	BmapFreeListMutex;	/* free-list mutex */
static	dlink_t	BmapFreeList;		/* free-list rooted here */
static	dlink_t	BmapActiveList;		/* active-list rooted here */

#define	BMAP_UNCACHED	((daddr_t) -1)	/* non-cached entry value */
#define	BMAP_MAXBLOCK	((2*1024/8)*1024) /* Max cachable logical block */

#define	BMAP_LOCK_LIST(s)	(s) = p_lock(&BmapFreeListMutex, SPLIMP)
#define	BMAP_UNLOCK_LIST(s)	v_lock(&BmapFreeListMutex, (s))

static	struct	BmapCache *BmapAlloc();
static	int	BmapEnable();
static	bool_t	BmapAllocMem();
static	bool_t	BmapGrow();
static	void	BmapFlush();
static	void	BmapDealloc();

#ifdef	DEBUG
static	int	BmapGrowFlush;		/* # times flushed in BmapMap */
static	int	BmapPurgeCalls;		/* # times called BmapPurge */
static	int	BmapNumPurge;		/* # times flushed in BmapPurge */
static	int	BmapNumFree;		/* # entries on free list */
#endif	DEBUG

/*
 * SysBmapCache() flavor mnemonics.  This needs to be in a header file.
 */

#define	BMC_OFF		0		/* turn off bmap caching (if on) */
#define	BMC_ON		1		/* turn on bmap caching */
#define	BMC_QUERY	2		/* query current state (on/off) */

/*
 * BmapBoot()
 *	Called at boot time to allocate and initialize BmapCache structures.
 */

BmapBoot()
{
	register struct BmapCache *bm;
	register int	i;

	init_lock(&BmapFreeListMutex, G_INOMIN);
	DL_INIT(&BmapFreeList);
	DL_INIT(&BmapActiveList);

	for (i = 0; i < NumBmapCache; i++) {
		bm = (struct BmapCache *) calloc(sizeof(struct BmapCache));
		init_lock(&bm->bm_mutex, G_INOMIN);
		DL_APPEND(&BmapFreeList, bm);
#ifdef	DEBUG
		++BmapNumFree;
#endif	DEBUG
	}
}

/*
 * SysBmapCache()
 *	Bmap cache system call.  Enable, disable, or query.
 *
 * Insist file-descriptor references VREG vnode for the UFS file-system.
 *
 * Returns 0 for success, else error number.
 */

SysBmapCache(fd, flavor)
	int	fd;			/* file descriptor index */
	int	flavor;			/* BMC_OFF, BMC_ON, BMC_QUERY */
{
	register struct vnode *vp;
	struct	file	*fp;
	int	error;

	/*
	 * Get object referenced by fd.  Insist on VREG UFS vnode.
	 */

	if (error = getvnodefp(fd, &fp))
		return error;
	vp = (struct vnode *) fp->f_data;
	if (vp->v_type != VREG || vp->v_op != &ufs_vnodeops)
		return EINVAL;

	/*
	 * Lower level routines expect exclusively locked inode.
	 */

	VN_LOCKNODE(vp);

	switch (flavor) {
	case BMC_ON:
		error = BmapEnable(VTOI(vp));
		break;
	case BMC_OFF:
		BmapDisable(VTOI(vp));
		break;
	case BMC_QUERY:
		u.u_r.r_val1 = (VTOI(vp)->i_bmcache != NULL);
		break;
	default:
		error = EINVAL;
		break;
	}

	VN_UNLOCKNODE(vp);

	return error;
}

/*
 * BmapEnable()
 *	Turn on bmap caching for an inode.
 *
 * Redundant calls are allowed and don't return an error.
 * Caller passes locked inode.
 *
 * Returns 0 for success, else error number.
 */

static int
BmapEnable(ip)
	struct	inode*	ip;
{
	if (ip->i_bmcache == NULL) {
		struct BmapCache* bm = BmapAlloc(ip);
		if (bm == NULL) {
			tablefull("bmap cache");
			return ENFILE;
		}
	}
	return 0;
}

/*
 * BmapDisable()
 *	Turn off bmap caching for an inode.
 *
 * Used in iinactive() when deactivate inode, and in SysBmapCache() to
 * implement the system call.
 *
 * Redundant calls are allowed and don't return an error.
 *
 * Caller holds inode exclusively locked.
 */

BmapDisable(ip)
	struct inode *ip;
{
	struct BmapCache *bm;

	if ((bm = ip->i_bmcache) != NULL) {
		ip->i_bmcache = NULL;
		BmapFlush(bm);
		BmapDealloc(bm);
	}
}

/*
 * BmapMap()
 *	Do a UFS bmap() operation, accessing and keeping cache up to date.
 *
 * Called instead of bmap() directly to use bmap cache if available.
 *
 * Caller holds inode locked (shared or exclusive).  This insures can't have
 * concurrent flush or truncate.
 */

/*VARARGS3*/
daddr_t
BmapMap(ip, bn, rwflg, size)
	struct	inode*	ip;
	daddr_t	bn;		/* file logical block number */
	int	rwflg;		/* B_READ or B_WRITE, optional B_NOCLR */
	int	size;		/* supplied only when rwflg == B_WRITE */
{
	register struct BmapCache *bm;
	daddr_t	nb;

	/*
	 * If a "direct block" or there is no bmap cache attached to this
	 * inode, do it the slow way.  Note this handles bn < 0 case, also.
	 * Direct blocks are fast anyhow (and this avoids issues of fragments
	 * extending).
	 *
	 * If important, could easily short-circuit direct block bmap for read.
	 * Write is harder due to fragment extending.
	 */

	if (bn < NDADDR || (bm = ip->i_bmcache) == NULL)
		return bmap(ip, bn, rwflg, size);

	ASSERT_DEBUG(bm->bm_ip == ip, "BmapMap: skew");

	/*
	 * If space isn't there, try once to re-establish, else fall back
	 * to "standard" way of doing things.
	 */

	if (bm->bm_daddr == NULL && !BmapAllocMem(bm))
		return bmap(ip, bn, rwflg, size);

	/*
	 * Handle logical block number beyond what's cached.
	 * Method depends on whether there are concurrent callers,
	 * which is only possible in the RWSEMA kernel.
	 */

	if ((bn - NDADDR) >= bm->bm_size) {
		/*
		 * Can have concurrent callers.
		 *
		 * If doing a write (==> exclusive lock on inode), attempt to
		 * grow.  If fail, flush the map so subsequent concurrent calls
		 * can know to re-establish.  This uses bm_addr==NULL as a
		 * "flag" of sorts passed between failed grow attempt and
		 * subsequent read map.
		 *
		 * If doing a read, can only be mapping beyond EOF, or where a
		 * write failed to grow the map.  Since failed grow on write
		 * flushes cache (and deletes memory), can't be bmap'ing where
		 * a write failed.  Thus safe to return (daddr_t)-1.
		 *
		 * Flushing map seems severe, but difficult to coordinate
		 * having readers attempt dynamic growth, without holding a
		 * lock while translating.  Don't want to hold a lock since
		 * this should be rare.  If most accesses to file are read maps
		 * (eg, DirectIO()), there won't be another chance to grow the
		 * map (eg, exclusive lock).  Also, if the grow fails, the
		 * system is low on resources and this gives a chance to
		 * "garbage collect".  Keep stat on how often this occurs and
		 * decide how severe it is.
		 */
		if (rwflg & B_READ) {
			ASSERT_DEBUG(bn>=ip->i_size/ITOV(ip)->v_vfsp->vfs_bsize,
					"BmapMap: bad read map");
			return (daddr_t) -1;
		}
		ASSERT_DEBUG(VN_LOCKEDNODE(ITOV(bm->bm_ip)),"BmapMap: !locked");
		if (!BmapGrow(bm, bn)) {
			BmapFlush(bm);
#ifdef	DEBUG
			++BmapGrowFlush;
#endif	DEBUG
			return bmap(ip, bn, rwflg, size);
		}
	}

	/*
	 * If have cached mapping, return it.
	 * Otherwise let bmap() do its thing and save the result.
	 *
	 * bmap() returns 0 if error, -1 if read and no space there, else
	 * physical fragment number.
	 *
	 * Could get smarter and cache mapping of holes.  However, expect
	 * read bmap'ing holes should be rare.
	 */

	nb = bm->bm_daddr[bn-NDADDR];
	if (nb == BMAP_UNCACHED) {
		nb = bmap(ip, bn, rwflg, size);
		if (nb > 0)
			bm->bm_daddr[bn-NDADDR] = nb;
	}

	return nb;
}

/*
 * BmapTrunc()
 *	File is being truncated.  Must keep cache coherent.
 *
 * Simple solution is to BmapFlush(), since don't expect this very often and
 * next RW of actual space will re-instantiate the memory for the cache.
 * Alternative is to zap the stuff no longer part of the file, possibly
 * shrink the memory.  If truncate to zero, then flush.
 *
 * For now, simple solution.
 *
 * Caller holds inode exclusively locked.
 */

/*ARGSUSED*/
BmapTrunc(ip, length)
	struct	inode	*ip;
	u_long	length;
{
	if (ip->i_bmcache)
		BmapFlush(ip->i_bmcache);
}

/*
 * BmapPurge()
 *	Attempt to purge memory from a bmap cache object.
 *
 * Scan active list looking for one where there is memory and can lock inode.
 * Then flush to free some memory, and move to tail of active list.
 *
 * Implementation assumes when inode goes inactive the bmap cache is purged;
 * this avoids problematic synchronization with iget() on IFREE inodes.
 * See TODO, above.
 *
 * Locking protocol similar to update(), using igrab().  igrab() fails if
 * last reference going away, but in this case iinactive() will flush the
 * memory anyhow.  Note that don't race with iinactive() calling
 * BmapDisable(), since if hold bmap cache list(s) locked, iinactive can't
 * complete.  Also, if iinactive() blocked on BmapFreeListMutex, will skip
 * the inode here since iinactive() holds it locked.  If igrab() succeeds,
 * concurrent VN_PUT can't happen (hold locked here) and VN_RELE will just
 * let it go.
 *
 * Called by sched() when memory is tight.  VERY basic memory reclamation hook.
 *
 * Returns true if found one and released some memory.
 */

bool_t
BmapPurge()
{
	register struct BmapCache *bm;
	spl_t	s;

#ifdef	DEBUG
	++BmapPurgeCalls;
#endif	DEBUG

	BMAP_LOCK_LIST(s);

	for (bm = (struct BmapCache *) BmapActiveList.dl_next;
	     bm != (struct BmapCache *) &BmapActiveList;
	     bm = (struct BmapCache *) bm->bm_link.dl_next) {
		if (bm->bm_daddr != NULL &&		/* if it has memory */
		    RWSEMA_IDLE(&bm->bm_ip->i_mutex) &&	/* and not busy */
		    igrab(bm->bm_ip)) {			/* and can grab it */
			struct inode *ip;
			/*
			 * Got one, hold inode locked and have reference.
			 * Flush cache and move to tail of active list.
			 */
			ip = bm->bm_ip;		/* safer, not really needed */
			remque(bm);
			DL_APPEND(&BmapActiveList, bm);
			BMAP_UNLOCK_LIST(s);
			BmapFlush(bm);
			IPUT(ip);
#ifdef	DEBUG
			++BmapNumPurge;
#endif	DEBUG
			return 1;
		}
	}

	BMAP_UNLOCK_LIST(s);
	return 0;
}

/*
 * BmapAllocMem()
 *	Attempt to allocate memory to hold bmap cache.
 *
 * Allocates memory to be able to map entire file.  If current size == 0,
 * don't allocate memory.
 *
 * Uses wmemall().
 *
 * Caller holds inode locked shared or exclusive.  If shared, can have
 * concurrent callers.  Hence, use bm_mutex.  (RWSEMA vnodes kernel only).
 *
 * Assumes i_size rounded up to block size is non-negative (guaranteed
 * by rwip()).
 *
 * Returns true if succeed, else false.
 */

static bool_t
BmapAllocMem(bm)
	register struct BmapCache *bm;
{
	register daddr_t *daddr;
	register int	i;
	int	bsize = ITOV(bm->bm_ip)->v_vfsp->vfs_bsize;
	int	szbytes;
	int	nbmap;
	spl_t	s;

	/*
	 * Determine # blocks mapped by indirect blocks in file.  If not large
	 * enough (or too large), don't allocate any memory.  Ok to test while
	 * outside lock(s), since if concurrent callers, inode size can't
	 * change (inode is held share-locked).
	 */

	nbmap = howmany(bm->bm_ip->i_size, bsize) - NDADDR;
	if (nbmap <= 0 || nbmap > BMAP_MAXBLOCK)
		return 0;
	szbytes = nbmap * sizeof(daddr_t);

	/*
	 * Allocate memory and insure all processors have coherent TLB's.
	 * There is a chance of a race here (multiple folks trying to do this),
	 * but non-harmful (those who loose will release the memory).
	 * Must do outside lock since FlushTLB() can block.
	 */

	daddr = (daddr_t *) wmemall(szbytes, 0);
	if (daddr == NULL)
		return 0;			/* fail */

	FlushTLB((caddr_t) daddr, szbytes);

	/*
	 * Lock, and if memory still doesn't exist try to allocate it.
	 * Might now exist since tested and called here, since could be
	 * racing with another process if hold inode shared-locked.
	 */

	BMAP_LOCK(bm, s);			/* in case racing */

	if (bm->bm_daddr == NULL) {
		/* 
		 * Zap allocated memory (indicate none cached).
		 * Careful to only write bm_daddr after it's
		 * all set up to avoid race with concurrent share lock.
		 *
		 * Note: "zap" loop could be done via string-op,
		 * behind some asm interface (ptefill() clone).
		 */
		bm->bm_size = roundup(szbytes,CLBYTES)/sizeof(daddr_t);
		for (i = 0; i < bm->bm_size; i++)
			daddr[i] = BMAP_UNCACHED;
		bm->bm_daddr = daddr;
		BMAP_UNLOCK(bm, s);
	} else {				/* lost race: delete memory */
		BMAP_UNLOCK(bm, s);
		wmemfree((caddr_t) daddr, szbytes);
	}


	/*
	 * Move to tail of active list -- make it less likley for BmapPurge()
	 * to get this one, since just allocated the memory.
	 */

	BMAP_LOCK_LIST(s);
	remque(bm);
	DL_APPEND(&BmapActiveList, bm);
	BMAP_UNLOCK_LIST(s);

	return 1;
}

/*
 * BmapGrow()
 *	Attempt to grow BmapCache object to include at least given logical
 *	block number.
 *
 * Implementation is intimate with wmemall()/wmemfree() -- "knows" about
 * Usrptmap[], etc.
 *
 * Caller insures bn >= 0 and beyond current cache size.
 * Caller holds relevant inode exclusively locked (thus no concurrent access
 * of cache data).
 *
 * Returns true for success, else false.
 */

static bool_t
BmapGrow(bm, bn)
	register struct BmapCache *bm;
	daddr_t	bn;
{
	register daddr_t *daddr;
	register int	i;
	long	oldpgs, newpgs;
	long	oldupt, newupt;
	int	blkcnt;

	ASSERT_DEBUG(bm->bm_daddr != NULL, "BmapGrow: null bm_daddr");
	ASSERT_DEBUG(((bm->bm_size * sizeof(daddr_t)) & CLOFSET) == 0,
							"BmapGrow: cursiz");

	bn -= NDADDR;			/* adjust away direct blocks */
	if (bn >= BMAP_MAXBLOCK)	/* ignore block too large */
		return 0;

	/*
	 * Allocate Usrptmap[] space to put all the stuff, allocate memory for
	 * the new pages.  If any allocation fails, unwind and return failure.
	 * This is a clone of wmemall() and ptexpand().  New array must be
	 * large enough to hold bn+1 entries (bn is zero-based index).
	 */

	oldpgs = clrnd(btop(bm->bm_size * sizeof(daddr_t)));
	newpgs = clrnd(btop(roundup((bn+1) * sizeof(daddr_t), CLBYTES)));
	ASSERT_DEBUG(newpgs > oldpgs, "BmapGrow: not growing");

	newupt = uptalloc(newpgs, 0);
	if (newupt == 0)
		return 0;
	if (!memall(&Usrptmap[newupt+oldpgs], (int)(newpgs-oldpgs), u.u_procp, CSYS)) {
		uptfree(newpgs, newupt);
		return 0;
	}

	/*
	 * Got the space and new memory, so will succeed.
	 * Move current pages to new location and release old mapping pte's.
	 * Note that this moves the pages to a different virtual address;
	 * it does not copy their contents.
	 */

	oldupt = btokmx((struct pte*) bm->bm_daddr);
	bcopy(	(caddr_t) &Usrptmap[oldupt],
		(caddr_t) &Usrptmap[newupt],
		(unsigned) (oldpgs * sizeof(struct pte))
	);
	uptfree(oldpgs, oldupt);

	/*
	 * Insure memory is properly accessible.
	 */

	bm->bm_daddr = (daddr_t *) kmxtob(newupt);	/* lives here now */
	vmaccess(&Usrptmap[newupt], (caddr_t) bm->bm_daddr, (int) newpgs);

	/*
	 * Zap the new space (indicate nothing cached) and set new size.
	 */

	blkcnt = ctob(newpgs - oldpgs) / sizeof(daddr_t);
	for (daddr = &bm->bm_daddr[bm->bm_size], i = 0; i < blkcnt; i++)
		*daddr++ = BMAP_UNCACHED;
	bm->bm_size = daddr - bm->bm_daddr;

	/*
	 * Insure everyones TLB's are up to date.
	 */

	FlushTLB((caddr_t) bm->bm_daddr, (int) ctob(newpgs));

	return 1;
}

/*
 * BmapFlush()
 *	Delete the memory held by the map.
 *
 * Used to reclaim memory.  Inode contines to reference BmapCache object.
 *
 * Caller holds relevant inode exclusively locked.
 */

static void
BmapFlush(bm)
	register struct BmapCache *bm;
{
	if (bm->bm_daddr != NULL) {
		wmemfree((char*) bm->bm_daddr, bm->bm_size * sizeof(daddr_t));
		bm->bm_daddr = NULL;
	}
}

/*
 * BmapAlloc()
 *	Allocate a BmapCache object and initialize to reference given inode.
 *
 * Caller holds inode exclusively locked.
 *
 * Returns NULL if fail.
 */

static struct BmapCache *
BmapAlloc(ip)
	struct inode *ip;
{
	register struct	BmapCache *bm;
	spl_t	s;

	BMAP_LOCK_LIST(s);
	if (!DL_EMPTY(&BmapFreeList)) {
		DL_DEQUEUE(&BmapFreeList, bm, struct BmapCache *);
		bm->bm_ip = ip;
		bm->bm_daddr = NULL;		/* no memory allocated */
		ip->i_bmcache = bm;
		DL_APPEND(&BmapActiveList, bm);
#ifdef	DEBUG
		--BmapNumFree;
#endif	DEBUG
	} else
		bm = NULL;
	BMAP_UNLOCK_LIST(s);

	return bm;
}

/*
 * BmapDealloc()
 *	De-allocate a BmapCache object.
 */

static void
BmapDealloc(bm)
	register struct	BmapCache *bm;
{
	spl_t	s;

	ASSERT_DEBUG(bm->bm_daddr == NULL, "BmapDealloc: daddr");

	BMAP_LOCK_LIST(s);
	bm->bm_ip = NULL;			/* be clean about it */
	remque(bm);				/* pull off active list */
	DL_APPEND(&BmapFreeList, bm);		/* re-insert on free-list */
#ifdef	DEBUG
	++BmapNumFree;
#endif	DEBUG
	BMAP_UNLOCK_LIST(s);
}

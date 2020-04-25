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
static	char	rcsid[] = "$Header: mmap_mfile.c 2.12 1991/11/13 22:21:24 $";
#endif

/*
 * mmap_mfile.c
 *	Procedures to manipulate "struct mfile"s.
 */

/* $Log: mmap_mfile.c,v $
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/fs.h"
#include "../ufs/inode.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/kernel.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/gate.h"
#include "../machine/mmu.h"
#include "../machine/mftpr.h"
#include "../machine/intctl.h"

struct	mfile	*mfile;			/* the array of mfile's */
int		nmfile;			/* # of mfile's */
struct	mfile	mfile_free;		/* head of free list */
lock_t		mfile_list;		/* lock list for alloc */

extern	int	mmreg_maxb;		/* max # buffers to use for sync */

/*
 * For mmreg_rw's references...
 */

extern	struct	pte	*mmreg_rwpte;		/* &Usrptmap[<idx>] */
extern	caddr_t		mmreg_rwva;		/* kmxtob(<idx>) */
extern	sema_t		mmreg_rwmutex;		/* for access to the mapping */

#ifdef	DEBUG
/*
 * Various statistics.
 */
int	num_mfile_free, min_mfile_free;
int	num_tacky_recycle;
#endif	DEBUG

/*
 * mfinit()
 *	Initialize mapped-file tables.
 *
 * If mapped IFREG files are to be binary configurable, this should become
 * the boot procedure of a pseudo-driver.
 */

mfinit()
{
	register struct mfile *mf;
	register int	ptx;
	register struct mfile *mfileNMFILE;

	mfileNMFILE = &mfile[nmfile];

	init_lock(&mfile_list, G_MFILE);

	mfile_free.mf_nfree = mfile_free.mf_pfree = &mfile_free;

	for (mf = mfile; mf < mfileNMFILE; mf++) {
		MF_INS_HEAD_FREE(mf);
		init_sema(&mf->mf_swmutex, 1, 0, G_MFILE);
#ifdef	DEBUG
		++num_mfile_free; ++min_mfile_free;
#endif	DEBUG
	}

	/*
	 * Allocate and initialize mapping pte's for mmreg_rw().
	 */

	ptx = uptalloc((long)btop(MAXBSIZE), 1);
	mmreg_rwpte = &Usrptmap[ptx];
	mmreg_rwva = (caddr_t)kmxtob(ptx);

	init_sema(&mmreg_rwmutex, 1, 0, G_MFILE);

	/*
	 * Allocate and clear the page of zeroes.
	 */

	(void) vmemall(zeropg, CLSIZE, &proc[0], CSYS);
	clearseg(PTETOPHYS(zeropg[0]));
	*(int*)(&zeropg[0]) |= RO|PG_R|PG_V;
	distcl(&zeropg[0]);
}

/*
 * mfile_alloc()
 *	Allocate a free struct mfile.
 *
 * Handles re-allocation of "tacky" VTEXT mappings.
 *
 * Looks thru free-list and takes 1st really free or tacky it finds.
 * Any "sticky"'s are moved to tail of list.
 */

struct mfile *
mfile_alloc()
{
	register struct mfile *mf;
	register struct vnode *vp;
	register struct mfile *nfree;
	struct	mfile	*first_sticky = NULL;
	spl_t	s;

	LOCK_MFILES(s);

	mf = mfile_free.mf_nfree;
	for (; mf != &mfile_free && mf != first_sticky; mf = nfree) {
		/*
		 * Remember next entry in case "sticky" moves one, below.
		 */
		nfree = mf->mf_nfree;
		/*
		 * If a really free one, take it.
		 */
		if (mf->mf_vp == NULL) {
			MF_RM_FREE(mf);
#ifdef	DEBUG
			if (--num_mfile_free < min_mfile_free)
				min_mfile_free = num_mfile_free;
#endif	DEBUG
			UNLOCK_MFILES(s);
			return(mf);
		}
		/*
		 * If a "sticky", move it to end of list; this avoids
		 * constantly skipping it when there are lots of tacky's.
		 * Ok to not hold locked vnode: since hold free-list locked
		 * the sticky/tacky can't be reclaimed concurrently.
		 */
		vp = mf->mf_vp;
		ASSERT(vp->v_flag & VTEXT, "mfile_alloc: !VTEXT");
		/*
		 *+ An mfile object on the free list was pointing to a
		 *+ vnode marked as having a valid text mapping.
		 */
		if (vp->v_flag & VSVTX) {
			MF_RM_FREE(mf);
			MF_INS_TAIL_FREE(mf);
			if (first_sticky == NULL)
				first_sticky = mf;
			continue;
		}
		/*
		 * Is a "tacky" ==> can reuse it if can grab vnode.
		 * Locked vnode ==> no race with mmreg_alloc().
		 */
		if (VN_TRYLOCKNODE(vp)) {
#ifdef	DEBUG
			++num_tacky_recycle;
			if (--num_mfile_free < min_mfile_free)
				min_mfile_free = num_mfile_free;
#endif	DEBUG
			MF_RM_FREE(mf);
			UNLOCK_MFILES(s);
			/*
			 * Ok to call mfile_rele() here without locking mf,
			 * since this is a tacky text, and hold vnode locked
			 * (callers attempt to reclaim text while holding
			 * vnode locked).
			 *
			 * Careful to get another vnode reference, since
			 * mfile_rele() may release last mfile reference.
			 */
			VN_HOLD(vp);		/* get another reference */
			mfile_rele(mf, 1);	/* release resources */
			VN_PUT(vp);
			return(mf);
		}
	}

	/*
	 * Didn't find one.  Too bad.
	 */

	UNLOCK_MFILES(s);

	tablefull("mfile");
	u.u_error = ENFILE;
	return(NULL);
}

/*
 * mfile_dealloc()
 *	Put an mfile on the free list.
 *
 * Caller cleared mf_vp if it's really free, else this is a "tacky" ref
 * to a mapped file.
 */

mfile_dealloc(mf)
	register struct mfile *mf;
{
	spl_t	s;

	mf->mf_flag = 0;			/* in case MF_UNMAP_RACE set */
	LOCK_MFILES(s);

	if (mf->mf_vp) {
		MF_INS_TAIL_FREE(mf);		/* "tacky" */
	} else {
		MF_INS_HEAD_FREE(mf);		/* really free */
	}
#ifdef	DEBUG
	++num_mfile_free;
#endif	DEBUG

	UNLOCK_MFILES(s);
}

/*
 * mfile_insque()
 *	Insert an mfile into a vnode's list of mfile.
 *
 * Must insert such that mfile sort is preserved (increasing starting
 * position), and update v_mapx if insert at front of list.
 *
 * Need to lock vnode map list to exclude (eg) mmreg_unmap(), mmreg_dup(),
 * etc which traverse this list.
 *
 * Caller holds vnode locked.
 * Caller doesn't hold vnode map list locked.
 */

#ifdef	DEBUG				/* XXX */
int	mfile_insque_unmapped = 0;	/* XXX */
#endif					/* XXX */

mfile_insque(vp, mf)
	register struct vnode *vp;
	register struct mfile *mf;
{
	register struct mfile *nmf;
	struct mfile *fmf;
	spl_t	s;

	VN_LOCK_MAP_LIST(vp, s);

	/*
	 * If vnode is not currently mapped, make this mfile the first entry.
	 * This code is a clone of part of mmreg_first_map(); this should be
	 * Caller handles worrying about "prot" and vnode time stamping.
	 */

	if (!VN_MAPPED(vp)) {
		ASSERT_DEBUG(VN_LOCKEDNODE(vp), "mfile_insque: vnode !locked");
		VN_UNLOCK_MAP_LIST(vp, s);
		vp->v_flag |= VMAPPED;
		vp->v_badmap = 0;	/* innocent until proven guilty */
		vp->v_mapx = mf-mfile;
		mf->mf_next = mf->mf_prev = mf;
#ifdef	DEBUG						/* XXX */
		++mfile_insque_unmapped;		/* XXX */
#endif							/* XXX */
		return;
	}

	/*
	 * Find entry to insert in front of.
	 */

	fmf = nmf = &mfile[vp->v_mapx]; 
	while (mf->mf_pos >= nmf->mf_pos) {
		nmf = nmf->mf_next;
		if (nmf == fmf)
			break;
	}

	/*
	 * Insert before found entry.  If at front of list, adjust v_mapx.
	 */

	insque(mf, nmf->mf_prev);
	if (nmf == fmf && mf->mf_pos < nmf->mf_pos)
		vp->v_mapx = (mf - mfile);

	VN_UNLOCK_MAP_LIST(vp, s);
}

/*
 * mfile_rele()
 *	Release resources associated with an mfile.
 *
 * Does NOT deallocate the mfile structure; caller does that if necessary.
 *
 * Used in un-mapping (MM_UNMAP), when re-using a tacky mfile, and when
 * flushing a given vnode of VTEXT mapping.
 *
 * Caller holds mf_vp locked.
 * Caller insures adequate mutex.  This can be by holding mfile locked
 * (MF_LOCK()), or holding vnode locked while releasing tacky text resources.
 *
 * Sets MF_UNMAP_RACE if deletion racing with another process trying to
 * create reference to this mfile.  This protocol blocks the other process,
 * has it release the mfile and assume it didn't hit.
 *
 * Also release reference to vnode.
 */

mfile_rele(mf, lastfd)
	register struct mfile *mf;
{
	register struct vnode	*vp = mf->mf_vp;
	spl_t	s;

	ASSERT_DEBUG(mf->mf_count == 0, "mfile_rele: count != 0");

	/*
	 * Sync it out, deleting reclaimable pages (even for VTEXT).
	 * Then free page-table.
	 */

	mfile_sync(mf, 1, 1, lastfd);
	mfile_freept(mf, 1);

	/*
	 * Remove from vnode queue and if was first entry,
	 * set up new "first" element.
	 *
	 * If this was last mfile mapping this vnode, turn off VMAPPED|VTEXT.
	 */

	VN_LOCK_MAP_LIST(vp, s);
	ASSERT_DEBUG(!(mf->mf_flag&MF_UNMAP_RACE), "mfile_rele: RACE");
	if (mf->mf_next != mf) {			/* won't empty */
		/*
		 * If removing 1st of list, have new "first".
		 */
		if (vp->v_mapx == (mf-mfile))
			vp->v_mapx = mf->mf_next - mfile;
		remque(mf);
	} else {					/* empty list ==> */
		/*
		 * Can't zap VMAPPED|VTEXT flags, since don't necessarily
		 * hold vnode locked.  But, can zap v_mapx which renders
		 * VMAPPED|VTEXT invalid.
		 */
		vp->v_mapx = VMAPX_NULL;		/* no longer mapped */
	}
	/*
	 * Racing with mmreg_new() or other list search?
	 */
	if (MF_BLOCKED(mf)) {
		mf->mf_flag |= MF_UNMAP_RACE;
#ifdef	DEBUG
		printf("mfile_rele: MF_UNMAP_RACE!\n");		/*XXX*/
#endif	DEBUG
	}
	VN_UNLOCK_MAP_LIST(vp, s);
	VN_RELE(vp);					/* done with refernce */

	mf->mf_vp = NULL;
}

/*
 * mfile_rmfree()
 *	Remove an mfile from the free-list.
 *
 * Used when a tacky is being reclaimed.
 */

mfile_rmfree(mf)
	register struct mfile *mf;
{
	spl_t	s;

	LOCK_MFILES(s);
	MF_RM_FREE(mf);
#ifdef	DEBUG
	if (--num_mfile_free < min_mfile_free) min_mfile_free = num_mfile_free;
#endif	DEBUG
	UNLOCK_MFILES(s);
}

/*
 * mfile_unref()
 *	Release a reference to a particular mfile.
 *
 * Caller takes care of releasing vnode ref if this turns off VMAPPED.
 *
 * Caller holds mfile referenced but not locked.
 *
 * If racing with an attempt to create new reference (eg, mmreg_new()), if
 * the other process wins it will bump reference count, and this code takes
 * simple path.  If win here, return with MF_UNMAP_RACE and let other process
 * unwind (don't dealloc the mfile here in this case).
 */

mfile_unref(mf, lastfd)
	register struct mfile *mf;
	bool_t		lastfd;			/* last fd for file closing? */
{
	struct	vnode	*vp;

	ASSERT_DEBUG(mf->mf_count > 0, "mfile_unref: UNMAP count");

	/*
	 * If last reference going away -- release resources and mfile,
	 * unless VTEXT: then just put it to tail of free-list.
	 * Otherwise (there are other ref's), decrement "in-core" count;
	 * if all remaining are swapped out, loose the PT again.
	 *
	 * Release VTEXT anyhow if vnode has no links -- no sense caching
	 * a map for a file with no name.  Could be careful of other
	 * refs to vnode (ala MFIGNOREMOD()), but no big win.
	 * Also delete map if the vnode is marked "bad".
	 *
	 * Note that decrementing count to zero before mfile_rele() allows
	 * pageout or lmemall(realloc) to call mmreg() with mf_count == 0.
	 * No problem, the mfile is locked due to locked vnode and is not
	 * yet on free list.
	 *
	 * Complication from using mf_swmutex to lock counts lives mostly
	 * here, in the "count == 0 case".
	 */

	MF_LOCK(mf);
	if (--mf->mf_count == 0) {		/* last ref going away */
		vp = mf->mf_vp;
		if ((vp->v_flag & (VTEXT|VNOLINKS)) != VTEXT || vp->v_badmap) {
			mfile_rele(mf, lastfd);
			if (mf->mf_flag & MF_UNMAP_RACE) {
				MF_UNLOCK(mf);
			} else {
				MF_UNLOCK(mf);
				mfile_dealloc(mf);
			}
		} else {
			/*
			 * Need to drop mfile lock and get it on the
			 * free-list atomicly.  Can be racing with a lookup,
			 * thus can't put it on free-list after releasing mfile
			 * lock.  Mostly an inline expansion of mfile_dealloc.
			 */
			spl_t	s;
			ASSERT(!(mf->mf_flag&MF_UNMAP_RACE),"mfile_unref:race");
			/*
			 *+ The kernel was releasing the last reference to
			 *+ an mfile data structure and noticed that a race
			 *+ condition had occurred.
			 */
			LOCK_MFILES(s);
			if (mf->mf_vp) {
				MF_INS_TAIL_FREE(mf);	/* "tacky" */
			} else {
				MF_INS_HEAD_FREE(mf);	/* really free */
			}
			MF_UNLOCK(mf);
#ifdef	DEBUG
			++num_mfile_free;
#endif	DEBUG
			UNLOCK_MFILES(s);
		}
	} else {				/* there are other refs */
		mfile_lccdec(mf);
		MF_UNLOCK(mf);
	}
}

/*
 * mfile_ccdec()
 *	Decrement in-memory count of mfile entry.  If hits zero, make sure
 *	page-table is cleaned and release resources.
 *
 * mfile_sync() cleans reclaimable dirty pages and detaches all pages,
 * rebuilding FOD pte's.
 *
 * mfile_lccdec() assumes caller already holds mfile locked.
 */

mfile_ccdec(mf)
	register struct mfile *mf;
{
	MF_LOCK(mf);
	mfile_lccdec(mf);
	MF_UNLOCK(mf);
}

static
mfile_lccdec(mf)
	register struct mfile *mf;
{
	if (--mf->mf_ccount == 0) {
		mfile_sync(mf, 1, 0, 0);
		mfile_freept(mf, 0);
	}
}


/*
 * mfile_ccbump()
 *	Bump in-memory count of mfile and if necessary try to bring it
 *	in from disk.
 *
 * May fail if don't require success and can't get resources.
 *
 * When do swap in of page-table, must be careful to avoid caller swapping
 * (mmreg_rw() doesn't really have a connection to the mfile, and if swapped
 * can deadlock the swapper).
 *
 * mfile_lccbump() assumes mfile is already locked.  Also assumes ok to drop
 * the lock (eg, if must_succeed), since caller has a reference to the mfile.
 *
 * Returns true for success, false for failure.
 */

mfile_ccbump(mf, must_succeed)
	register struct mfile *mf;
	bool_t	must_succeed;
{
	int	val;

	MF_LOCK(mf);

	val = mfile_lccbump(mf, must_succeed);

	MF_UNLOCK(mf);

	return val;
}

mfile_lccbump(mf, must_succeed)
	register struct mfile *mf;
	bool_t	must_succeed;
{
	while (mf->mf_ccount == 0) {
		ASSERT_DEBUG(mf->mf_pt == NULL, "mfile_lccbump: skew");
		if (mfile_allocpt(mf, 0)) {
			swap(	u.u_procp, mf->mf_ptdaddr,
				&Usrptmap[btokmx(mf->mf_pt)],
				(int)ctob(MFSZPT(mf)),
				B_READ|B_PAGET, swapdev_vp
			);
			break;
		} else if (must_succeed) {
			/*
			 * If insist on success, must be sure to not
			 * deadlock swapper -- ie, must drop swmutex
			 * in case this mfile is part of a chain.
			 */
			MF_UNLOCK(mf);
			p_sema(&lbolt, PRSWAIT);
			MF_LOCK(mf);
		} else
			break;
	}

	if (mf->mf_pt)
		++mf->mf_ccount;

	return mf->mf_ccount;
}

/*
 * mfile_allocpt()
 *	Allocate a page-table for a mapped file.
 *
 * Conditionally succeed -- allows swapin to fail.
 *
 * If must_succeed is true, can allocate and hold Usrptmap[] space while
 * waiting on memory.  Don't think this is a problem.
 */

mfile_allocpt(mf, must_succeed)
	register struct mfile *mf;
	bool_t		must_succeed;
{
	mf->mf_pt = (struct pte *) wmemall((int)ctob(MFSZPT(mf)), must_succeed);
	return((int) mf->mf_pt);
}

/*
 * mfile_freept()
 *	Free page-table resources of a mfile, optionally writing out
 *	dirty page-table.
 *
 * Assumes caller has exclusive access to mfile's page-table and structure,
 * and mfile_sync'd the file if necessary.
 */

mfile_freept(mf, rel_swap)
	register struct mfile *mf;
	bool_t	rel_swap;			/* release PT swap space */
{
	register long	ptx;
	register long	pts;

	pts = MFSZPT(mf);

	/*
	 * If mfile has no page-table (ie, mmreg_flush(1) already deleted it),
	 * this must be a full release.
	 */

	if (mf->mf_pt == NULL) {
		ASSERT(rel_swap && !(mf->mf_flag & MF_PTDIRTY), "mfile_freept");
		/*
		 *+ mfile_freept was called to free the mapping information
		 *+ associated with an mfile object.  The object was marked
		 *+ as having a modified page table; however, the page table
		 *+ had already been freed.
		 */
		swapfree(ctod(pts), mf->mf_ptdaddr);
		return;
	}
	ptx = btokmx(mf->mf_pt);

	/*
	 * If freeing swap-space, easy since mfile_sync() cleaned it.
	 * Else if dirty, write PT.
	 * Otherwise (keeping space, but already wrote PT): NOP -- mfile_sync()
	 * already released reclaimables.
	 */

	if (rel_swap)
		swapfree(ctod(pts), mf->mf_ptdaddr);
	else if (mf->mf_flag & MF_PTDIRTY) {
		swap(u.u_procp, mf->mf_ptdaddr, &Usrptmap[ptx], (int)ctob(pts),
			B_WRITE|B_PAGET, swapdev_vp);
		mf->mf_flag &= ~MF_PTDIRTY;
	}

	wmemfree((caddr_t)mf->mf_pt, (int)ctob(pts));
	mf->mf_pt = NULL;
}

/* 
 * mfile_sync()
 *	Insure an mfile is clean on disk, optionally detach pages.
 *
 * `detach' ==> re-build FOD pte's and free pages.
 *
 * !detach could be used by msync() function (maybe pass in start
 * position and size).  Currently no calls with !detach; some
 * ASSERT's are too strong.
 *
 * Somewhat complicated ZFOD handling (here and in mfile_wrkl()) is
 * to avoid need of allocating new resource, and to allow higher
 * performance sync.
 *
 * Assumes caller has blocked swap-in's, swap-out's, other msync's, and
 * any unmaps -- ie, there is at most one caller of this procedure for a
 * given mfile at a time.  mmreg_rw() and mfile_unref() are disjoint due
 * to holding locked vnode.  Unmap vs swapout can't occur since each
 * precludes the other.  mmreg_rw() vs swap can't occur since mmreg_rw()
 * uses mfile_ccbump/dec().  New ref needs locked vnode, and calls
 * mfile_ccbump().
 *
 * NOTE: could have simpler version when writem==0; there are not any
 * dirty pages being written, and this is used ONLY to release reclaims.
 * Might lower overhead when recycle tacky or swap out.
 *
 * NOTE: avoids doing writes if vnode is marked "bad" -- had an IO error
 * or stale text mapping.  This is a bit heavy-handed; when/if this needs
 * to be smarter, could have IO errors mark only the relevant pages (but
 * this becomes non-deterministic).
 */

static
mfile_sync(mf, detach, last_ref, last_fd)
	struct	mfile	*mf;
	bool_t		detach;			/* detach or keep pages? */
	bool_t		last_ref;		/* last ref going away? */
	bool_t		last_fd;		/* last fd, too ?? */
{
	register struct	cmap	*c;
	register struct pte	*pte;
	register struct	buf	*fb = NULL;	/* 1st buffer in IO chain */
	register struct	buf	*lb;		/* last buffer in chain */
	register struct	buf	*bp;		/* IO buffer */
	bool_t		writem = 1;		/* assume writing */
	bool_t		dozfod = 0;		/* assume not doing ZFOD's */
	struct	vnode	*vp = mf->mf_vp;
	int		numbuf;
	int		count;
	int		pgcnt;
	daddr_t		blkno;
	struct	buf	*swbuf_alloc();

#ifdef	lint
	lb = NULL;			/* lint can't figure out code below */
#endif
	/*
	 * If a VTEXT map, nothing to write and no ZFOD's.
	 * If last ref to the map is going away, may not have to sync
	 * if last file-descriptor is also going away and vnode has
	 * no other references (or name).  Vnode with no other ref's
	 * has ref-count == 2: one for mfile's and one for file-desc.
	 */

	if (vp->v_flag & VTEXT) {
		if (mf->mf_pt == NULL)		/* mmreg_flush(1) did it */
			return;
		writem = 0;
	} else if (last_ref) {
		if (MFIGNOREMOD(last_fd, vp))
			writem = 0;
		else
			dozfod = 1;
	}

	/* 
	 * Run thru page-table writing modified pages,
	 * rebuilding FOD pte's as necessary.
	 *
	 * Since noone executes on this page-table, sufficient to
	 * look only at 1st pte of a cluster.
	 *
	 * There should NOT be any valid pte's (all refs should be gone).
	 *
	 * IO's are started asynch, and pages released/detached when the
	 * IO completes.
	 *
	 * This can't race with mmreg_fill(), since mmreg_fill() caller
	 * holds locked vnode and mf_ccount >= 1.
	 */

	count = (mf->mf_zsize ? mf->mf_zsize : mf->mf_size);
	for (pte = mf->mf_pt; count > 0; pte += pgcnt, count -= pgcnt) {
		pgcnt = CLSIZE;				/* in case no action */

		if (pte->pg_fod) {
			if (!dozfod || !writem || (*(int *)pte & PG_FZERO) == 0)
				continue;
			LOCK_MEM;
			*(int *) pte |= PG_M;
#ifdef	DEBUG
			c = PTETOCMAP(zeropg[0]);
			ASSERT(	c->c_refcnt != 0
			&&	!c->c_dirty
			&&	!c->c_pageout, "mfile_sync: zeropg");
			c = NULL;
#endif	DEBUG
		} else if (*(int *) pte == PG_INVAL) {	/* un-mapped hole */
			continue;
		} else {
			LOCK_MEM;
			if (pte->pg_fod) {		/* lost race */
				UNLOCK_MEM;
				pgcnt = 0;
				continue;
			}
			c = PTETOCMAP(*pte);
			ASSERT_DEBUG(c>=cmap && c<ecmap, "mfile_sync: bad pg");
		}

		/*
		 * If modified, write it and nearby others.
		 * Have to check both pte and cmap entry, in case MMU_MBUG is
		 * fixed (reclaimable pte will be !pg_m but cmap says c_dirty).
		 */

		if (writem && (pte->pg_m || c->c_dirty)) {

			/*
			 * If map is bad, don't bother writing.  Careful to not
			 * detach FOD pages.  Can't have ref to zeropg here.
			 */

			if (vp->v_badmap) {
				if (detach && !pte->pg_fod) {
					ASSERT(PTEPF(*pte) != PTEPF(zeropg[0]), "mfile_sync: zeropg");
					/*
					 *+ When trying to free a mapped page 
					 *+ attached to a vnode marked as
					 *+ having a bad map, the kernel 
					 *+ found a reference to zeropg. 
					 */
					mfile_sync_detach(pte, c);
				}
				UNLOCK_MEM;
				continue;
			}

			/*
			 * Find a set of contiguous dirty pages.
			 */

			if (mfile_wrkl(mf, pte, 0, dozfod, &pgcnt, &blkno) > 0)
				panic("mfile_sync: wrkl");
				/*
				 *+ The kernel discovered an unexpected page 
				 *+ in the page table for a mapped file
				 *+ section while synching that mapped
				 *+ file's modified pages to disk.
				 */

			/*
			 * Release memory lock, get a buf-header, and
			 * write'em (asynch).  Make sure have at least
			 * one buf-header, but get others if can.  If
			 * can't get a new one, reuse a previous.
			 *
			 * mmreg_maxb imposes limit on # swbuf headers
			 * used here.
			 */

			UNLOCK_MEM;

			if (fb == NULL) {
				bp = fb = swbuf_alloc(1);
				numbuf = 1;
			} else if (numbuf<mmreg_maxb && (bp=swbuf_alloc(0))) {
				lb->b_forw = bp;
				++numbuf;
			} else {
				bp = fb;
				if (fb = bp->b_forw)
					lb->b_forw = bp;
				else
					fb = bp;
				biowait(bp);
				/*
				 * If got an error, mark map bad.
				 */
				if (bp->b_flags & B_ERROR)
					vp->v_badmap |= MM_IOERR;
				mfile_sync_rlse(bp, detach);
			}
			bp->b_forw = NULL;
			bp->b_bufsize = pgcnt;
			lb = bp;

			swapio(bp, u.u_procp, blkno, pte, ctob(pgcnt),
					B_DIRTY|B_WRITE, vp->v_devvp);
		} else {
			/*
			 * If detaching, re-build FOD pte and release claim
			 * on page.  Skip any ZFOD's, since they're only done
			 * on last-reference, in which case page-table is
			 * going away anyhow.
			 */
			if (detach && PTEPF(*pte) != PTEPF(zeropg[0]))
				mfile_sync_detach(pte, c);
			UNLOCK_MEM;
		}
	}

	/*
	 * Clean up any outstanding IO.  If got an error, mark map bad.
	 *
	 * biowait() sets u.u_error; clearing u.u_error avoids confusion
	 * at upper levels.  Someday u.u_error will go away entirely.
	 */

	while (bp = fb) {
		fb = fb->b_forw;
		biowait(bp);
		if (bp->b_flags & B_ERROR)
			vp->v_badmap |= MM_IOERR;
		mfile_sync_rlse(bp, detach);
		swbuf_free(bp);
	}
	u.u_error = 0;
}

/*
 * mfile_wrkl()
 *	Look for adjacent modified pages for writing.
 *
 * Returns delta before pte of 1st pte, number of pages, and 1st block#.
 * Limit to KLMAX klusters.
 *
 * Called with memory locked.
 */

mfile_wrkl(mf, pte0, is_pageout, dozfod, pcnt, pbn)
	struct	mfile	*mf;
	struct	pte	*pte0;			/* pte that must be written */
	bool_t		is_pageout;		/* is this pageout calling? */
	bool_t		dozfod;			/* should do ZFOD's ?? */
	int		*pcnt;			/* return # pages */
	daddr_t		*pbn;			/* return 1st block number */
{
	register struct cmap *c;
	register struct pte *pte;
	register int	incr;
	register daddr_t blkno;
	register int	count;
	struct	pte	*max_pte = mf->mf_pt + mf->mf_size;
	struct	pte	*spte[2];
	daddr_t		blkno0;
	int		pass;

	/*
	 * Look forward then backwards to find modified pte's for
	 * contiguous disk blocks, reclaim into mfile PT.
	 *
	 * 1st page is trivially part of this.
	 *
	 * pageout() can race with mmreg_fill() adding to page-table.
	 * Is a don't care, since vinifod() turns ON PG_FOD bit with
	 * 1st write, and pageout() isn't interested in FOD's.  Must
	 * still check for !PG_INVAL, though.  This cleans up a bit
	 * with finer-grain memory locking, since MM_PGOUT case will
	 * lock mf_swmutex (thus, mmreg_fill() can, too).
	 */

	if (*(int*)pte0 & PG_FZERO) {
		blkno = blkno0 = PTE_TO_BLKNO(*pte0);
		ASSERT(dozfod, "mfile_wrkl: !dozfod");
		/*
		 *+ Arguments to mfile_wrkl() are inconsistent.
		 *+ The type of the page table entry passed in
		 *+ is inconsistent with the operations to be
		 *+ performed on it.
		 */
	} else
		blkno = blkno0 = PTETOCMAP(*pte0)->c_blkno;
	count = 0;
	pte = pte0;
	incr = CLSIZE;

	for (pass = 0; pass <= 1; pass++) {

		while(	count < KLMAX
		&&	pte < max_pte
		&&	pte >= mf->mf_pt
		&&	*(int*)pte != PG_INVAL) {

			if (pte->pg_fod == 0) {

				/*
				 * !FOD: take if dirty and matching blkno.
				 */

				if ((c = PTETOCMAP(*pte))->c_blkno == blkno
				&&  (pte->pg_m || c->c_dirty)) {

					if (is_pageout) {
						/*
						 * If pageout already has it,
						 * stop here (rare, but can
						 * happen).
						 */
						if (c->c_pageout)
							break;
						/*
						 * Pageout didn't unlink pte0's
						 * page, etc.
						 */
						if (c->c_refcnt == 0) {
							CM_UNLINK(c);
						}
						c->c_pageout = 1;

					} else {

						/*
						 * "sync" -- actually reclaim
						 * into PT.
						 */

						if (c->c_refcnt == 0
						&&  c->c_pageout == 0) {
							CM_UNLINK(c);
						}
						++c->c_refcnt;
						pte->pg_v = 1;
					}

				} else
					break;

			} else {

				/*
				 * FOD: only interested if ZFOD with matching
				 * blkno and doing ZFOD.
				 */

				if (dozfod
				&&  (*(int *)pte & PG_FZERO)
				&&  PTE_TO_BLKNO(*pte) == blkno) {

					/*
					 * Get a ref to common zero page.
					 * No need to set MF_PTDIRTY here,
					 * since only `dozfod' if last ref
					 * going away.
					 */

					ASSERT(!is_pageout, "mfile_wrkl: zfod");
					/*
					 *+ The pageout daemon tried to page 
					 *+ out a "zero fill on demand" page.
					 */
					c = PTETOCMAP(zeropg[0]);
					copycl(pte, zeropg);
					++c->c_refcnt;
					/* pte->pg_v = 1; */

				} else
					break;
			}

			c->c_dirty = 0;				/* will clean */
			pte->pg_m = 0;				/* will clean */
			pte += incr;
			blkno += ctod(incr);
			++count;
		}

		/*
		 * Set up for backwards pass.  If !pageout, 2nd
		 * pass will not really happen (but want spte[1] set).
		 */

		spte[pass] = pte;
		pte = pte0 - CLSIZE;
		blkno = blkno0 - ctod(CLSIZE);
		incr = -CLSIZE;
	}

	/*
	 * Set up return values and done.  spte[0] is one cluster after
	 * last, spte[1] is one cluster before first.
	 */

	pte = spte[1] + CLSIZE;				/* 1st pte */
	*pcnt = count * CLSIZE;
	ASSERT_DEBUG(count >= 1, "mfile_wrkl: count");
	*pbn = blkno0 - ctod(pte0 - pte);
	return(pte0 - pte);
}

/*
 * mfile_sync_rlse()
 *	IO done on a buffer -- release (maybe detach) claim on pages.
 *
 * Assumes pte's can't move (eg, mfile can't get swapped).
 * Caller does NOT hold memory locked.
 *
 * NOTE: current implementation always passes detach == true.
 */

static
mfile_sync_rlse(bp, detach)
	struct	buf	*bp;
	bool_t		detach;
{
	register struct pte *pte = bp->b_un.b_pte;
	register int	pgcnt = bp->b_bufsize;

	LOCK_MEM;
	for (; pgcnt > 0; pgcnt -= CLSIZE, pte += CLSIZE) {
		memfree1(pte);
		/*
		 * As in mfile_sync, ignore pages used to sync a ZFOD.
		 */
		if (detach && PTEPF(*pte) != PTEPF(zeropg[0]))
			mfile_sync_detach(pte, PTETOCMAP(*pte));
	}
	UNLOCK_MEM;
}

/*
 * mfile_sync_detach()
 *	Detach a page and reconstruct FOD pte.
 *
 * Assumes pte's can't move (eg, mfile can't get swapped).
 * Caller holds memory lists locked.
 */

static
mfile_sync_detach(pte, c)
	register struct pte *pte;
	register struct cmap *c;
{
	/*
	 * Can race with pageout process in trying to clean pages.  Use
	 * `c_iowait' and intrans_wake() protocol to synch this.
	 * Have to sync to avoid (low probability) case of drop page,
	 * re-fault, mod, write, all before pageout's IO makes it.
	 *
	 * `c' remains valid since pageout won't free the page if
	 * c_iowait is on.  Page can't get reclaimed/etc, since this is
	 * only in-memory reference (detach ==> going away or swapping
	 * out)...  And since hold vnode locked (and have reference) it
	 * can't dissappear, the page-table can't move, and mfile has
	 * nailed state (no attach/detach/swap).
	 */

	ASSERT_DEBUG(c->c_refcnt == 0, "mfile_sync_detach: detach ref");
	ASSERT_DEBUG(!pte->pg_v, "mfile_sync_detach: dt v");

	if (c->c_pageout) {
		u.u_procp->p_spwait = &proc[c->c_iondx];
		c->c_iondx = u.u_procp - proc;
		c->c_iowait = 1;
		UNLOCK_MEM;
		++l.cnt.v_intrans;
		p_sema(&u.u_procp->p_pagewait, PSWP);
		LOCK_MEM;

		ASSERT_DEBUG(!c->c_pageout && !c->c_iowait &&
			     c->c_refcnt == 0 && !pte->pg_v,
						"mfile_sync_detach: oops!");

		/*
		 * Free page here, since must redo FOD pte below (ie,
		 * can't have pageout() free it). It's possible for
		 * the page to have been reclaimed, written and freed
		 * after pageout started cleaning it. Clear the dirty
	 	 * field anyway since it's going away.
		 */

		c->c_gone = 1;
		c->c_dirty = 0;
		CM_INS_HEAD_CLEAN(c);
	} else {
		/*
		 * If dirty, move to clean free-list.  Page can be dirty if
		 * not writing on last reference (eg, no links to vnode).
		 */

		c->c_gone = 1;
		if (c->c_dirty) {
			CM_UNLINK_DIRTY(c);
			c->c_dirty = 0;
			CM_INS_HEAD_CLEAN(c);
		}
	}

	ASSERT(c->c_holdfod && c->c_type == CMMAP, "mfile_sync_detach: skew");
	/*
	 *+ In mfile_sync_detach, the kernel encountered
	 *+ an internal inconsistency in page table
	 *+ data structures.
	 */

	vredoFOD(c, pte);
}

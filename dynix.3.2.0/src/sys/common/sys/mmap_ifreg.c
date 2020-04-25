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
static	char	rcsid[] = "$Header: mmap_ifreg.c 2.27 1991/11/05 22:20:12 $";
#endif

/*
 * mmap_ifreg.c
 *	Support for mapping regular files.
 *
 * See mmap_mfile.c for struct mfile manipulation functions.
 *
 * Since use mf_swmutex for bump/decr mf_ccount can insure no memory deadlocks
 * for swap in/out -- ie, regardless of state of locked vnodes/etc, pageout
 * process can get to mfile entries and swap them out.  Also, swapin is
 * independent of vnode state, avoiding swapin latencies.
 *
 * FUTURES:
 *	i_lsize to track logical size of file and allow truncate of mapped file.
 *	Or, map non-block-size chunks using rwip() (eg, EOF).
 *
 *	support IFBLK files (almost identical to IFREG, use different vinifod).
 *
 *	put VMAPPED bit (maybe others: RO, RW indication) in disk copy of inode,
 *	and arrange to have fsck notice this (since there may have been some
 *	zfod blocks that aren't initialized).  Also, could create tool to
 *	update mapped file from vmcore image...
 */

/*
 * mmreg/mfile deadlock is instance where read() of file over MAP_PRIVATE space
 * mapped from same file, and touch all pages, deadlocks when pagein() tries to
 * release mapping, since process already holds vnode locked).
 * 
 * Can't use vnode semaphore to lock mfile list, nor use this to mutex
 * mmreg_unref() calls.  Approach is to use the mf_swmutex semaphore, lock
 * mf_count *and* mf_ccount with this.  Careful to avoid race between
 * mmreg_new() and mmreg_unmap()/mfile_unref(): when mfile is removed from vnode
 * "v_mapx" list, hold vnode mutex (spin lock).  Can race with mmreg_new() (who
 * must hold list locked and do PSVL when "hit" on mfile), and let mmreg_new()
 * unwind this.
 * 
 * Also, mfile_rele() can't fiddle v_flag, since it doesn't hold vnode locked.
 * Thus, zap v_mapx (while holding v_mutex), and modify places that interrogate
 * VMAPPED and VTEXT to sense bad v_mapx instead; ie, if !VN_MAPPED(vp), then ok
 * to zap flags.
 *
 * Need to hold reference to the vnode per mfile attached to the vnode.  This
 * is due to v_mapx only really being stable when the vnode is (hard) locked
 * (unless !VN_MAPPED(), where soft locking vnode keeps it not mapped).  Various
 * algorithms would race on releasing reference to vnode.  MF_UNLOCK_VP() goes
 * away as a result (good result; this was sort of a violation of the locking
 * and reference-count model).
 * 
 * Hold vnode locked when call mmreg_new().  This insures only one process can
 * be there at a point in time for given vnode, and mutually excludes other uses
 * of vnode (eg, truncate).  However, can have concurrent mmreg_new() with
 * mmreg_unmap().
 *
 * Note: is there any reason we can't cache a writable mapping?  Looks like
 * mfile_rele() will handle it, just incurring IO when released.  Should test
 * this.
 */

/* $Log: mmap_ifreg.c,v $
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

/*
 * Define mapping operations for regular files.
 */

static	int	mmreg_new();
static	int	mmreg_dup();
static	int	mmreg_unmap();
static	int	mmreg_swpout();
static	int	mmreg_swpin();
static	int	mmreg_refpg();
static	int	mmreg_derefpg();
static	int	mmreg_realloc();
static	int	mmreg_pgout();
static	int	mmreg_stat();
static	int	mmreg_err();

struct	mapops	mmap_reg = {
	mmreg_new,		/* create a new map */
	mmreg_dup,		/* dup ref to map (fork) */
	mmreg_unmap,		/* release reference to map */
	mmreg_swpout,		/* swap out ref to map */
	mmreg_swpin,		/* swap in ref to map */
	mmreg_refpg,		/* get ref to page */
	mmreg_derefpg,		/* remove page ref */
	mmreg_realloc,		/* drop reclaim link to page */
	mmreg_pgout,		/* page-out page */
	mmreg_stat,		/* get information about the map */
	mmreg_err,		/* import an error to the map */
};

/*
 * VOP_MARK() may need to be generalized for file systems other
 * than ufs.  Note: nfs doesn't care.
 */

#define	VOP_MARK(VP, FLAGS) \
	{ if ((VP)->v_op == &ufs_vnodeops) IMARK(VTOI(VP), FLAGS); }

struct	mfile	*mmreg_newmf();
struct	mfile	*mmreg_fill();
struct	mfile	*mfile_alloc();

/*
 * For mmreg_rw's references...
 */

struct	pte	*mmreg_rwpte;		/* &Usrptmap[<idx>] */
caddr_t		mmreg_rwva;		/* kmxtob(<idx>) */
sema_t		mmreg_rwmutex;		/* for access to the mapping */

#ifndef lint
int mmap_ifreg_installation_error;	/* set for conf/conf_ofs.c */
#endif

/* 
 * mmreg_new()
 *	Initial set up of IFREG file mapping.
 *
 * Called in mmap() to obtain handle on internal structures and
 * determine if legal mapping.
 *
 * Allows extension of already mapped file if RW ref, else no extension.
 * This to better support run-time expansion of shared, paged memory (eg,
 * for "shmalloc").
 *
 * Insist on mapping full blocks only and dis-allow truncate to insure
 * space doesn't go away.  Could do "i_lsize" later to be more transparent.
 *
 * If (prot & PROT_EXEC), arranges that relevant node is cached, insists
 * on no writeable references to the file, and doesn't round up size (other
 * than to page-boundary).
 *
 * Returns MM_PAGED and sets *handlep if all goes well, else returns
 * error number.  Stores relevant struct mfile pointer in *handlep.
 *
 * NOTE: if enough processes call this concurrently wanting big enough
 * new maps, it's possible to deadlock system memory since once the page-table
 * is allocated there is no "swap hook" for the structure until after it's
 * filled out and returned to caller.  The problem is worse if there are
 * multiple mfiles (could ccbump one and block waiting for memory for
 * another).  In practice this shouldn't happen.  Could add "swap hook" in
 * callers U-area so partially filled out mfile's could be swapped even
 * though process doesn't really reference it yet.
 *
 * Caller holds vp locked.
 */

static
mmreg_new(vp, pos0, size0, prot, handlep)
	struct	vnode	*vp;			/* locked node to map */
	u_long		pos0;			/* HW pages */
	int		size0;			/* HW pages */
	int		prot;			/* PROT_BITS */
	u_long		*handlep;		/* return if succeed */
{
	register struct mfile *mf;
	struct	mfile	*savemf = NULL;		/* in case error unwind */
	struct	mfile	*fmf;
	register u_long pos;
	register int	size;
	register int	esize;
	long		osize;
	int		boff;
	long		bsize;
	int		error;
	struct	vattr	vattr;
	spl_t		s;

	/*
	 * Insure don't wrap around size of file.
	 * Note that this check used to be done in mmreg_newmf() but
	 * was factored out here to cover a missed case: mapping an
	 * illegal offset after a previous legal mapping of a file.
	 */

	if (((long)ctob(pos0) < 0) || ((long)ctob(pos0+size0) <= 0)) {
		u.u_error = EFBIG;
		return (u.u_error);
	}

	/*
	 * Make pos, size file-system block aligned.
	 */

	bsize = vp->v_vfsp->vfs_bsize;
	if (bsize < CLBYTES) {
	    return (EDEADLK);	/* OAS would for example */
	}

	boff = btop((int)ctob(pos0) % bsize);

	pos = pos0 - boff;

	if (prot & PROT_EXEC) {
		/*
		 * Round up size to page-boundary in file-system block.
		 */
		size = btop(roundup(ctob(size0+boff), CLBYTES));
		/*
		 * Check for RW reference to this vnode, if not already
		 * in use for execution.  Caller guaranteed !PROT_WRITE.
		 * If VTEXT ever goes on, the only way it goes off is via
		 * mmreg_rele(), or observation the file is no longer mapped
		 * (eg, mmreg_rw()).  If VTEXT set, there cannot be any RW
		 * file-descriptors; need not worry about stale VTEXT.
		 */
		if ((vp->v_flag & VTEXT) == 0 && vp->v_count != 1
		&&  (error = check_rw_fd(vp)))
			return (error);
	} else {
		/*
		 * Note that if asking for write access, must have had RW
		 * file-desc, thus !VTEXT (since openning a RW file-desc
		 * insists on turning off mapping of VTEXT vnode).
		 */
		ASSERT_DEBUG((prot&PROT_WRITE) == 0 || (vp->v_flag&VTEXT) == 0,
				"mmreg_new: writeable VTEXT");
		size = btop(roundup(ctob(size0+boff), bsize));
	}

	if (error = VOP_GETATTR(vp, &vattr, u.u_cred))
		return (error);

	/*
	 * If vnode map is marked as a "bad": try to purge if a VTEXT,
	 * else return error; avoid continued use of known bad map.
	 */

	if (vp->v_badmap && VN_MAPPED(vp) &&
	    (!(vp->v_flag & VTEXT) || !mmreg_rele(vp)))
		return (vp->v_badmap & MM_STALETEXT) ? ETXTBSY : EIO;

	/*
	 * If not already mapped, do the simple thing.
	 */

	if (!VN_MAPPED(vp))
		return mmreg_first_map(vp, &vattr, pos, size, prot, handlep);

	/*
	 * On each iteration, lock vnode -> mfile list and find relevant
	 * existing mfile.  This is extra traveral in most cases (eg, unless
	 * race with mmreg_unmap()), but typically there are very few mfile's
	 * so no problem.  If "hit", need to drop list lock and grab matching
	 * mfile lock (use "psvl").  This can race with mmreg_unmap() releasing
	 * the last reference on the mfile.  If it does race, mmreg_unmap()
	 * sets MF_UNMAP_RACE flag.
	 */

	osize = vattr.va_size;				/* in case error */
	pos0 = pos;
	size0 = size;

	for (; size > 0; pos += esize, size -= esize) {

		/*
		 * Find entry within or before which `pos' fits.
		 * v_mapx is index of first mfile table entry for the vnode.
		 * Can race with mmreg_unmap() making last mfile for the
		 * vnode go away.
		 */

		VN_LOCK_MAP_LIST(vp, s);
		if (!VN_MAPPED(vp)) {		/* lost mmreg_unmap() race */
			VN_UNLOCK_MAP_LIST(vp, s);
			ASSERT_DEBUG(vattr.va_size==osize, "mmreg_new: osize");
			return mmreg_first_map(vp, &vattr, pos0, size0, prot, handlep);
		}

		fmf = mf = &mfile[vp->v_mapx];
		while (pos >= mf->mf_pos + mf->mf_size) {
			if (mf->mf_next == fmf)
				break;
			mf = mf->mf_next;
		}
		ASSERT_DEBUG(mf->mf_vp == vp, "mmreg_new: skew");

		if (pos >= mf->mf_pos + mf->mf_size) {
			/*
			 * If beyond largest offset, need a new entry.
			 * Note: once drop map list lock, maps can go away;
			 * mfile_insque() handles this.  Can't have concurrent
			 * new maps being created for this vnode, since caller
			 * holds vnode locked.
			 */
			VN_UNLOCK_MAP_LIST(vp, s);
			if (mf = mmreg_newmf(vp, pos, size, prot, &vattr)) {
				mfile_insque(vp, mf);
				*handlep = (u_long) mf;
				savemf = mf;
			}
			break;
		} else if (pos >= mf->mf_pos) {
			/*
			 * If overlap, bump part that overlaps.
			 * If count==0, reclaim the "tacky" mapping.
			 * Handle race with mmreg_unmap().
			 */
			MF_XFR_LOCK(mf, vp, s);
			if (mf->mf_flag & MF_UNMAP_RACE) {
				ASSERT(!MF_BLOCKED(mf), "mmreg_new: blocked");
				/*
				 *+ The kernel discovered an internal 
				 *+ inconsistency in trying to handle a race 
				 *+ condition in the mapping subsystem.
				 */
				MF_UNLOCK(mf);
				mfile_dealloc(mf);
				esize = 0;
				continue;
			}
			if (++mf->mf_count == 1) {
				mfile_rmfree(mf);
				/*
				 * Read in PT if mmreg_flush(1) deleted it.
				 * Can't have other refs, since just came off
				 * free list and hold locked vnode.
				 */
				if (mf->mf_ccount == 0)
					(void) mfile_lccbump(mf, 1);
			} else
				(void) mfile_lccbump(mf, 1);
			esize = MFOVERLAP(mf, pos, size);
			if (fmf = mmreg_fill(mf, pos, esize, prot, &vattr)) {
				*handlep = (u_long) fmf;
				savemf = fmf;
			} else {
				if (savemf == NULL)
					savemf = mf;
				size -= esize;
				MF_UNLOCK(mf);
				break;
			}
			MF_UNLOCK(mf);
		} else {
			/*
			 * Fit new piece in front.
			 * Note: once drop map list lock, maps can go away;
			 * mfile_insque() handles this.
			 */
			VN_UNLOCK_MAP_LIST(vp, s);
			esize = imin((int)(mf->mf_pos - pos), size);
			if (mf = mmreg_newmf(vp, pos, esize, prot, &vattr)) {
				mfile_insque(vp, mf);
				*handlep = (u_long) mf;
				savemf = mf;
			} else
				break;
		}
	}

	/*
	 * Any error ==> unwind.
	 */

	if (u.u_error) {
		error = u.u_error;
		if (savemf)
			mmreg_unmap(savemf, pos0, size0 - size, prot);
		if (vattr.va_size != osize) {
			/*
			 * Truncate will not actually succeed, since
			 * VMAPPED.  Thus file grows virtually; keep this
			 * here for good form, waiting for the time when
			 * truncate works on mapped files.
			 */
			vattr_null(&vattr);
			vattr.va_size = (u_long)osize;
			(void) VOP_SETATTR(vp, &vattr, u.u_cred);
		}
		return (error);
	}

	/*
	 * Made it -- if asking for PROT_EXEC, turn on VTEXT to remember
	 * this.  Note that VTEXT is "sticky" -- it persists even if the
	 * "exec" ref goes away and leaves only RO refs (implementation
	 * restriction, shouldn't hurt much).  Can't create a RW ref to a
	 * VTEXT vnode since openning an RW file-desc will turn off VTEXT
	 * mapping if possible.
	 *
	 * Also set vnode flags to reflect assumed access (done here
	 * to avoid overhead/vnode-lock in refpg,pgout), and can't do in
	 * mmreg_unmap() since don't hold vnode locked).
	 */

	if (prot & PROT_EXEC) {
		vp->v_flag |= VTEXT;
	}
	if (prot & PROT_WRITE) {
		VOP_MARK(vp, IUPD|ICHG);
	}
	if (prot & PROT_READ) {
		VOP_MARK(vp, IACC);
	}

	return (MM_PAGED);
}

/*
 * mmreg_first_map()
 *	Vnode not already mapped.  Create first mapping.
 *
 * Called with locked vnode.
 */

static
mmreg_first_map(vp, va, pos, size, prot, handlep)
	struct	vnode	*vp;			/* locked node to map */
	struct	vattr	*va;			/* current file attributes */
	u_long		pos;			/* HW pages */
	int		size;			/* HW pages */
	int		prot;			/* PROT_BITS */
	u_long		*handlep;		/* return if succeed */
{
	struct	mfile	*mf;
	long	osize = va->va_size;

	mf = mmreg_newmf(vp, pos, size, prot, va);
	if (mf == NULL) {
		int error = u.u_error;
		if (va->va_size != osize) {		/* if extended */
			vattr_null(va);
			va->va_size = (u_long)osize;
			(void) VOP_SETATTR(vp, va, u.u_cred);
		}
		return (error);
	}

	/*
	 * Vnode is now mapped and has another reference.
	 * If PROT_EXEC, also turn on VTEXT.
	 * Must turn off VTEXT otherwise, since mmreg_unmap() can't do this.
	 *
	 * Also set vnode flags to reflect assumed access (done here
	 * to avoid overhead/vnode-lock in refpg,pgout), and can't do in
	 * mmreg_unmap() since don't hold vnode locked).
	 */

	vp->v_flag |= VMAPPED;
	vp->v_badmap = 0;		/* innocent until proven guilty */
	if (prot & PROT_EXEC)
		vp->v_flag |= VTEXT;
	else
		vp->v_flag &= ~VTEXT;
	if (prot & PROT_WRITE) {
		VOP_MARK(vp, IUPD|ICHG);
	}
	if (prot & PROT_READ) {
		VOP_MARK(vp, IACC);
	}

	*handlep = (u_long) mf;
	vp->v_mapx = mf-mfile;		/* ok to not lock, 1st such */

	mf->mf_next = mf->mf_prev = mf;

	return (MM_PAGED);
}

/*
 * mmreg_newmf()
 *	Create a new mapping chunk for an vnode.
 *
 * Caller has already rounded pos and size to appropriate boundaries
 * (pos to file-sys block boundary , size to file-sys block boundary or
 * page boundary if PROT_EXEC).
 *
 * Returns new table entry, or NULL (and u.u_error) if some problem.
 * Updates va->va_size if file size changes.
 *
 * Assumes caller holds locked vnode.
 */

static struct mfile *
mmreg_newmf(vp, pos, size, prot, va)
	register struct	vnode	*vp;
	u_long		pos;			/* HW pages */
	int		size;			/* HW pages */
	register int	prot;			/* PROT_BITS */
	struct	vattr	*va;
{
	register struct mfile *mf;
	register long	osize;
	register u_int	ptdelta;		/* pos delta off PT start */
	struct	vattr	vattr;
	daddr_t		bn;
	long		nsize = ctob(pos+size);
	int		bsize = vp->v_vfsp->vfs_bsize;

	ptdelta = pos & (NPTECL-1);

	/*
	 * If needed, round vnode size to multiple of file-sys block size;
	 * vinifod() will take care of creating any ZFOD space in file.
	 * Don't allow this if not writing.
	 * Also, insure caller isn't over-extending file-size limit.
	 */
	
	osize = va->va_size;

	if (nsize > osize) {
		if (nsize > u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
			psignal(u.u_procp, SIGXFSZ);
			u.u_error = EFBIG;
			return (NULL);
		}
		if ((prot & PROT_WRITE) == 0) {
			u.u_error = EACCES;
			return (NULL);
		}
		if (osize % bsize) {
			VOP_BMAP(vp, (osize / bsize), (struct vnode *)0,
				 &bn, B_WRITE, bsize);
			if (bn < 0)
				return (NULL);
			vattr_null(&vattr);
			va->va_size = vattr.va_extfile = ((osize / bsize) + 1) * bsize;
			(void) VOP_SETATTR(vp, &vattr, u.u_cred);
			vp->v_flag |= VMAPSYNC;
		}
	}

	/*
	 * Allocate a free mapped-file table entry, and fill out for this
	 * vnode.  Set up mf_pos and mf_size to be page-table aligned.
	 */

	if ((mf = mfile_alloc()) == NULL)
		return (NULL);
	mf->mf_pos = pos - ptdelta;
	mf->mf_size = (size + ptdelta + NPTECL-1) & ~(NPTECL-1);
	mf->mf_ptdaddr = swapalloc((long)ctod(MFSZPT(mf)));
	if (mf->mf_ptdaddr == 0) {
		mfile_dealloc(mf);
		u.u_error = ENOMEM;
		return (NULL);
	}

	/*
	 * Got table entry and have swap space for page-table.
	 * Fill out entry and allocate in-memory page table.
	 * Need to zap unused parts of page-table.
	 *
	 * Hold vnode locked here, so no further calls can get
	 * here for this vnode.
	 *
	 * Calling process is swappable during this.
	 */

	mf->mf_count = mf->mf_ccount = 1;
	mf->mf_vp = vp;
	mf->mf_flag = MF_PTDIRTY;		/* if swap, need to write PT */

	(void) mfile_allocpt(mf, 1);

	bzero((caddr_t)mf->mf_pt, ptdelta * sizeof(struct pte));
	bzero(	(caddr_t) (mf->mf_pt + ptdelta + size),
		((unsigned)mf->mf_size - (ptdelta+size)) * sizeof(struct pte));

	/*
	 * If starting at 0'th pte in the file, set mf_zsize to allow
	 * some optimizations.
	 */

	if (ptdelta == 0)
		mf->mf_zsize = size;
	else
		mf->mf_zsize = 0;			/* no optimizations */

	/*
	 * Init mapping of vnode.   Also sync out file if need be.
	 * Could use mmreg_fill() here, but *know* only this portion of
	 * page-table needs to be init'd (thus avoid zeroing all of it above).
	 *
	 * If RO, fail if some space doesn't exist.
	 * If RW, allow vinifod() to create ZFOD space.
	 *
	 * Is faster to do VMAPSYNC test and VOP_FSYNC() explicitly here,
	 * since call vinifod() *after* increase size of file 
	 */

	if (vp->v_flag & VMAPSYNC)
		VOP_FSYNC(vp, u.u_cred);

	if (nsize > va->va_size) {		/* checked PROT_WRITE above */
		vattr_null(&vattr);
		va->va_size = vattr.va_extfile = (long)ctob(pos+size);
		(void) VOP_SETATTR(vp, &vattr, u.u_cred);
	}

	if (!vinifod(mf->mf_pt + ptdelta, vp, (daddr_t)pos/CLSIZE, (size_t)size, 0,
							prot & PROT_WRITE)) {
		mfile_freept(mf, 1);
		mf->mf_vp = NULL;			/* really free it */
		mfile_dealloc(mf);
		if (u.u_error == 0)
			u.u_error = EACCES;		/* RO and sparse */
		return (NULL);
	}

	/*
	 * Assert a reference to the vnode per mfile attached.
	 */
	VN_HOLD(vp);

	return (mf);
}

/*
 * mmreg_fill()
 *	Possibly fill-in part of an existing mfile's page-table in response to
 *	new mmap() request.
 *
 * Returns argument "mf", or NULL if fail.
 * Updates va->va_size if file size changes.
 *
 * Caller holds locked vnode.
 * Caller also holds mf "locked" (MF_LOCK()) which mutexes against concurrent
 * mmreg_unmap() (since mmreg_unmap() doesn't hold vnode locked).
 */

static struct mfile *
mmreg_fill(mf, pos, size, prot, va)
	struct	mfile	*mf;
	u_long		pos;
	int		size;
	int		prot;
	struct	vattr	*va;
{
	register struct pte *pte;
	register int	pgcnt;
	register long	pgbsize;
	struct	vnode	*vp;
	u_long		pos0;
	int		size0;
	long		nsize;
	struct	vattr	vattr;

	/*
	 * If pos & size fit within mf_pos + mf_zsize, then already
	 * know it's all there.
	 */

	if (pos + size <= mf->mf_pos + mf->mf_zsize)
		return (mf);

	pos0 = pos;
	size0 = size;
	vp = mf->mf_vp;
	if (vp->v_flag & VMAPSYNC)			/* if dirty */
		VOP_FSYNC(vp, u.u_cred);

	pgbsize = CLSIZE;

	ASSERT_DEBUG(((pos|size) & (pgbsize-1)) == 0, "mmreg_fill: skew");

	/*
	 * Scan the page-table and fill in any holes covered by this new
	 * map.  Must look at each page in case doing a RO map of VTEXT
	 * vnode that didn't map to file-sys block boundary.  Could note
	 * this in mf_flag and set pgbsize = btop(vp->v_vfsp->vfs_bsize);
	 * don't expect this to be a big performance diff.
	 *
	 * This can race with pageout()'s call to mfile_wrkl().  See comments
	 * there for why this is ok.  Also, can't race with mfile_sync()'s
	 * call of mfile_wrkl() since caller holds locked vnode and bumped
	 * mf_ccount.
	 */

	pte = MFOFFTOPTE(mf, pos - mf->mf_pos);
	for (; size > 0; size -= pgcnt, pte += pgcnt, pos += pgcnt) {
		pgcnt = pgbsize;
		if (*(int *) pte != PG_INVAL)
			continue;
		while (pgcnt < size && *(int *) (pte + pgcnt) == PG_INVAL)
			pgcnt += pgbsize;
		/*
                 * Map the hole.  Extend file first, if need (only if RW).
                 */
		nsize = ctob(pos + pgcnt);
                if (nsize > va->va_size) {
                        if ((prot & PROT_WRITE) == 0) {
                                u.u_error = EACCES;
                                return (NULL);
                        }
			/*
                         * Test for exceeding file-limit.
                         */
                        if (nsize > u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
                                psignal(u.u_procp, SIGXFSZ);
                                u.u_error = EFBIG;
                                return (NULL);
                        }
                        vattr_null(&vattr);
                        va->va_size = vattr.va_extfile = nsize;
                        (void) VOP_SETATTR(vp, &vattr, u.u_cred);
		}
		if (nsize > u.u_rlimit[RLIMIT_FSIZE].rlim_cur) {
			psignal(u.u_procp, SIGXFSZ);
			u.u_error = EFBIG;
			return (NULL);
		}

		if (!vinifod(pte, vp, (daddr_t)pos/CLSIZE, (size_t)pgcnt, 0,
							prot & PROT_WRITE)) {
			if (u.u_error == 0)
				u.u_error = EACCES;	/* RO and sparse */
			return (NULL);
		}
	}

	/*
	 * Success!  If new stuff extends mf_zsize contiguously, bump mf_zsize;
	 * otherwise we have a sparse map and can't assume contig from mf_pos.
	 */

	if (mf->mf_zsize != 0 && pos0 <= mf->mf_pos + mf->mf_zsize)
		mf->mf_zsize = (pos0 - mf->mf_pos) + size0;
	else
		mf->mf_zsize = 0;			/* sparse map */

	return (mf);
}

/*
 * mmreg_dup()
 *	Dup ref to a map during fork().
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmreg_dup(mf, off, size)
	register struct mfile	*mf;
	u_long		off;			/* HW pages */
	int		size;			/* HW pages */
{
	register struct vnode	*vp = mf->mf_vp;
	int	esize;
	spl_t	s;

	/*
	 * Bump ref-count on all overlapped mfile's.
	 * Use loop as in mmreg_new(), but simplified -- mapping can't go
	 * away since caller holds a reference over the range.
	 *
	 * Could optimize: once find first intersection, guaranteed the set
	 * of intersections is contiguous and can't go away.  Thus, no need
	 * to scan list from beginning each time.  Careful: list can change
	 * once it's unlocked.  However, typically only a few mfile's.
	 */

	for (; size > 0; off += esize, size -= esize) {
		VN_LOCK_MAP_LIST(vp, s);
		ASSERT_DEBUG(VN_MAPPED(vp), "mmreg_dup: !mapped");
		mf = &mfile[vp->v_mapx];
		while (!MFINTERSECT(mf, off, size)) {
			mf = mf->mf_next;
			ASSERT_DEBUG(mf!=&mfile[vp->v_mapx], "mmreg_dup: loop");
		}
		MF_XFR_LOCK(mf, vp, s);
		ASSERT_DEBUG(mf->mf_count > 0, "mmreg_dup: count");
		ASSERT_DEBUG(!(mf->mf_flag&MF_UNMAP_RACE), "mmreg_dup: RACE");
		++mf->mf_count;
		(void) mfile_lccbump(mf, 1);
		esize = MFOVERLAP(mf, off, size);
		MF_UNLOCK(mf);
	}
}

/*
 * mmreg_unmap()
 *	Release reference to a map.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmreg_unmap(mf, off, size, prot)
	register struct mfile	*mf;
	u_long	off;			/* HW pages */
	int	size;			/* HW pages */
	int	prot;			/* PROT_READ|PROT_WRITE */
{
	register struct vnode	*vp = mf->mf_vp;
	int	esize;
	spl_t	s;

	/* 
	 * For each overlap'd mfile, decrement counts, release things
	 * if about to hit zero, possibly doing IO to sync up file.
	 *
	 * Upper levels took care of insuring M-bit isn't passed in
	 * for RO files (independent of MMU_MBUG).
	 *
	 * Need to insure don't swap since caller assumes this.
	 * mfile_unref() deals with this.
	 *
	 * mfile_unref() also deals with dropping reference to vnode if last
	 * mfile goes away.
	 */

	for (; size > 0; off += esize, size -= esize) {
		VN_LOCK_MAP_LIST(vp, s);
		ASSERT_DEBUG(VN_MAPPED(vp), "mmreg_unmap: !mapped");
		mf = &mfile[vp->v_mapx];
		while (!MFINTERSECT(mf, off, size)) {
			mf = mf->mf_next;
			ASSERT_DEBUG(mf!=&mfile[vp->v_mapx], "mmreg_unmap: loop");
		}
		VN_UNLOCK_MAP_LIST(vp, s);
		esize = MFOVERLAP(mf, off, size);
		mfile_unref(mf, prot & PROT_LASTFD);
	}
}

/*
 * mmreg_swpout()
 *	Swap out overlapping maps of given vnode.
 *
 * To avoid need to hold locked vnode, handles list carefully.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmreg_swpout(mf, off, size)
	register struct mfile *mf;
	register u_long	off;			/* HW pages */
	register int	size;			/* HW pages */
{
	/*
	 * `mf' can't go away since there is at least one referencing
	 * process (ie, the one trying to go out).  Similarly, any
	 * overlap'd mfile can't go away.  Since [off,off+size) is
	 * represented by a contiguous set of mfile's the list links
	 * won't change as long as only looking at intersecting mfile's.
	 */

	ASSERT_DEBUG(mf->mf_count > 0, "mmreg_swpout: count");
	ASSERT(MFINTERSECT(mf, off, size), "mmreg_swpout: skew");
	/*
	 *+ The kernel discovered an internal inconsistency while
	 *+ swapping out a mapped region.
	 */

	while (MFINTERSECT(mf, off, size)) {
		if (mf->mf_pos <= off)
			break;
		mf = mf->mf_prev;
	}
	
	while (MFINTERSECT(mf, off, size)) {
		mfile_ccdec(mf);
		if (mf->mf_pos + mf->mf_size >= off + size)
			break;
		mf = mf->mf_next;
	}
}

/*
 * mmreg_swpin()
 *	Try to swap in a set of mfile's for a process.
 *
 * Careful traversal of list avoids need to lock vnode (ie, list
 * can't change in relevant range during this since [off,off+size)
 * guaranteed to exist).
 *
 * Returns 0 for success else error number.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmreg_swpin(mf, off, size)
	register struct mfile *mf;
	register u_long	off;			/* HW pages */
	register int	size;			/* HW pages */
{
	/*
	 * Note that off thru off+size are in a contiguous set of mfile
	 * entries, since entries are sorted by increasing position.
	 * Thus, given any intersecting one can look thru list as long
	 * as still intersect.
	 *
	 * Can find smallest by looking backwards until find 1st to intersect.
	 */

	ASSERT_DEBUG(mf->mf_count > 0, "mmreg_swpin: count");
	ASSERT(MFINTERSECT(mf, off, size), "mmreg_swpin: skew");
	/*
	 *+ The kernel found an internal inconsistency while
	 *+ swapping in a mapped region.
	 */

	while (MFINTERSECT(mf, off, size)) {
		if (mf->mf_pos <= off)
			break;
		mf = mf->mf_prev;
	}
	
	while (MFINTERSECT(mf, off, size)) {
		if (!mfile_ccbump(mf, 0)) {
			if (mf->mf_pos > off)
				mmreg_swpout(mf->mf_prev, off,
							(int)(mf->mf_pos-off));
			return (ENOMEM);
		}
		if (mf->mf_pos + mf->mf_size >= off + size)
			break;
		mf = mf->mf_next;
	}

	return (0);					/* success! */
}

/*
 * mmreg_refpg()
 *	Return the physical address of a page, finding appropriate
 *	struct mfile if there are several.
 *
 * Returns physical address, or MM_BADMAP bits for errors.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmreg_refpg(mf, off)
	register struct mfile	*mf;
	register u_long	off;			/* HW pages */
{
	ASSERT_DEBUG(mf->mf_count > 0, "mmreg_refpg: count");

	/*
	 * Start at handle's mfile, since it is largest mapped
	 * for the caller.
	 *
	 * Take advantage of knowledge that set of mfile's for
	 * given mapping are contiguous in mfile list sorted
	 * by increasing mf_pos, and this part of list can't
	 * change as long as this reference exists.
	 */

	if (off >= mf->mf_pos) {
		while (off >= mf->mf_pos + mf->mf_size)
			mf = mf->mf_next;
	} else {
		do
			mf = mf->mf_prev;
		while (off < mf->mf_pos);
	}

	ASSERT_DEBUG(MFINTERSECT(mf, off, CLSIZE), "mmreg_refpg: SKEW");

	return (mmreg_ref_page(mf, off - mf->mf_pos));
}

/*
 * mmreg_derefpg()
 *	Remove a reference to a page.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmreg_derefpg(mf, off, mod, lastfd)
	register struct mfile	*mf;
	register u_long	off;			/* HW pages */
	bool_t		mod;			/* was page modified? */
	bool_t		lastfd;			/* is this last fd ref? */
{
	register struct	pte	*pte;

	ASSERT_DEBUG(mf->mf_count > 0, "mmreg_derefpg: count");

	/*
	 * Find pte and decrement it ref-cnt (no detach) at it.
	 * Use similar loop/etc to mmreg_refpg().
	 */

	if (off >= mf->mf_pos) {
		while (off >= mf->mf_pos + mf->mf_size)
			mf = mf->mf_next;
	} else {
		do
			mf = mf->mf_prev;
		while (off < mf->mf_pos);
	}
	ASSERT_DEBUG(MFINTERSECT(mf,off,CLSIZE), "mmreg_derefpg: SKEW");

	/*
	 * If last fd reference going away, maybe ignore modified state.
	 * Else, worry about setting modified bit.
	 *
	 * Not clear MFIGNOREMOD is a big win here, but doesn't cost much.
	 */

	pte = MFOFFTOPTE(mf, off - mf->mf_pos);

	LOCK_MEM;
	if (MFIGNOREMOD(lastfd, mf->mf_vp))
		*(int *) pte &= ~PG_M;
	else if (mod)
		*(int *) pte |= PG_M;
	memfree1(pte);
	UNLOCK_MEM;
}

/*
 * mmreg_realloc()
 *	Release claim on reclaimable page -- it's getting reallocated.
 *
 * Called holding memory locked.
 */

static
mmreg_realloc(mf, off)
	struct mfile	*mf;
	u_long		off;			/* HW pages */
{
	register struct	pte	*pte;

	pte = MFOFFTOPTE(mf, off);

	ASSERT_DEBUG(!pte->pg_v, "mmreg_realloc: valid");
	ASSERT_DEBUG(PTETOCMAP(*pte)->c_holdfod, "mmreg_realloc: !fod");

	vredoFOD(PTETOCMAP(*pte), pte);
}

/*
 * mmreg_pgout()
 *	Pageout process found a mapped page and wants it written out.
 *	Find adjacent pages if possible and tell pageout about them.
 *
 * Must munlink(c), turn off c_dirty, turn on c_pageout.
 *
 * Called by pageout process while holding memory locked.
 *
 * Returns error without doing anything if there's a problem with the page,
 * else returns 0 and has filled out *pgo.
 */

static
mmreg_pgout(mf, off, pgo)
	register struct mfile	*mf;
	u_long		off;			/* HW pages */
	register struct	pgout	*pgo;
{
	register struct	pte	*pte;

	if (mf->mf_vp->v_badmap)
		return(mf->mf_vp->v_badmap);

	pte = MFOFFTOPTE(mf, off);
	pgo->po_pte = pte - mfile_wrkl(mf, pte, 1, 0,
						&pgo->po_cnt, &pgo->po_blkno);
	pgo->po_devvp = mf->mf_vp->v_devvp;
	return(0);
}

/*
 * mmreg_stat()
 *	Return information about a map.
 *
 * Information is heuristic -- subject to change even while being returned
 * here.
 */

static
mmreg_stat(mf, statp)
	struct mfile	*mf;
	struct	map_stat *statp;
{
	ASSERT_DEBUG(mf->mf_count > 0, "mmreg_stat: count");

	statp->ms_count = mf->mf_count;
	statp->ms_ccount = mf->mf_ccount;
}

/*
 * mmreg_err()
 *	Assert an error on a page of a map.
 *
 * Interface passes in offset of page and # pages; current implementation
 * marks entire map "bad".
 */

static
mmreg_err(mf, off, size, error)
	struct mfile	*mf;
	u_long		off;			/* HW pages */
	size_t		size;			/* HW pages */
	int		error;
{
	mf->mf_vp->v_badmap |= error;
#ifdef	lint
	lint_ref_int((int) off);
	lint_ref_int((int) size);
#endif	lint
}

/* 
 * mmreg_ref_page()
 *	Pagein wants to reference a page.
 *
 * If already valid, bump ref-cnt and done.
 * If reclaimable, reclaim it and done.
 * If ZFOD, handle.
 * Else, need to FOD it.
 *
 * Code originated from pagein() CTEXT case.
 *
 * Returns physical address of page, after bumping reference count.
 *
 * Caller does NOT hold memory-lists locked.
 */

static
mmreg_ref_page(mf, off)
	struct mfile	*mf;
	u_long		off;			/* # HW pages from mf_pos */
{
	register struct pte	*pte;
	register struct pte	*npte;
	register struct cmap	*c;
	struct	proc	*p = u.u_procp;
	struct	vnode	*vp = mf->mf_vp;
	struct	buf	*bp;
	int		klsize;
	int		delta;
	int		i;
	daddr_t		bn;
	bool_t		zfod;
	struct	buf	*swbuf_alloc();

	ASSERT_DEBUG(off < mf->mf_size, "mmreg_ref_page: big off");

	for (;;) {
		/*
		 * If mapping went bad (IO error, stale text (NFS)),
		 * return bogus reference and reason for failure.
		 */

		if (vp->v_badmap)
			return(vp->v_badmap);

		/*
		 * If page already exists, make another reference.
		 */

		pte = MFOFFTOPTE(mf, off);

		LOCK_MEM;

		if (pte->pg_v) {
			c = PTETOCMAP(*pte);
			ASSERT_DEBUG(PTEPF(*pte) != 0, "mmreg_ref_page: unalloc");
			ASSERT_DEBUG(c->c_refcnt != 0, "mmreg_ref_page: skew");
			++c->c_refcnt;
			UNLOCK_MEM;
			u.u_ru.ru_minflt++;
			break;
		}

		/*
		 * If page is reclaimable, reclaim it; might be in-transit.
		 */

		if (pte->pg_fod == 0) {

			/*
			 * If already intransit, have to wait for it.
			 */

			c = PTETOCMAP(*pte);
			ASSERT_DEBUG(c >= cmap && c < ecmap,
						"mmreg_ref_page: bad reclaim");
			if (c->c_intrans) {
				p->p_spwait = &proc[c->c_iondx];
				c->c_iondx = p - proc;
				c->c_iowait = 1;
				UNLOCK_MEM;
				++l.cnt.v_intrans;
				p_sema(&p->p_pagewait, PSWP);
				continue;
			}

			if (c->c_pageout == 0) {
				CM_UNLINK(c);
			}
			ASSERT_DEBUG(c->c_refcnt==0,"mmreg_ref_page: reclaim cnt");

			/*
			 * Bump ref-count, keep c_holdfod on.  c->c_blkno
			 * already has correct block number.
			 */

			c->c_refcnt = 1;
			if (c->c_dirty) {
				pte->pg_m = 1;
				c->c_dirty = 0;
				l.cnt.v_pgdrec++;
			}
			*(int*)pte |= PG_V;

			UNLOCK_MEM;

			u.u_ru.ru_minflt++;
			l.cnt.v_pgrec++;
			l.cnt.v_pgfrec++;
			break;
		}

		/*
		 * Is ZFOD or FOD from file.
		 *
		 * Insist on having a max-size kluster of pages available,
		 * blocking (swappable) until this is true.  Thus can pre-page
		 * more than just one file-sys block, if the blocks are
		 * contiguous.
		 *
		 * Still hold memory locked here.
		 *
		 * Even in local VM model, since mfile is shared if block
		 * waiting for memory (may even swap) the state of the
		 * mfile pte we're after might have changed.  Thus, re-start
		 * fault after wakeup.
		 */

		if (freemem < CLSIZE * KLMAX) {
			UNLOCK_MEM;
			WAIT_MEM;
			continue;
		}

		/*
		 * Now can get memory for the page(s).
		 * Get a page and set c_holdfod so we can remember blkno
		 * in case page gets reallocated.
		 *
		 * Set intrans to tell other users of the mfile the
		 * page is on it's way in.
		 */

		bn = PTE_TO_BLKNO(*pte);
		zfod = *(int *)pte & PG_FZERO;

		lmemall(pte, CLSIZE, (struct proc *)0,
				CMMAP, &mmap_reg, off, (u_long) mf);

		c = PTETOCMAP(*pte);
		c->c_holdfod = 1;
		c->c_intrans = 1;
		c->c_blkno = bn;

		/*
		 * If ZFOD just clear the page.
		 */

		if (zfod) {
			/*
			 * Obey the "intrans" protocol and clear page
			 * while not holding memory lists locked.
			 */
			UNLOCK_MEM;
			clearseg(PTETOPHYS(*pte));
			LOCK_MEM;
			*(int *) pte |= PG_V|PG_M;
			if (c->c_iowait)
				intrans_wake(c);
			c->c_intrans = 0;
			/*
			 * Page-table is now dirty again -- if swap out,
			 * must re-write PT (since changed a ZFOD pte
			 * into a FOD pte).  Ok to set this here, since
			 * other fuss can't happen now (can't be getting
			 * swapped right now).
			 */
			mf->mf_flag |= MF_PTDIRTY;
			UNLOCK_MEM;
			l.cnt.v_zfod += CLSIZE;
			break;
		}

		/*
		 * Is FOD from file.
		 *
		 * Try for pre-paging adjacent FOD pages.
		 * Since the pte's loose their pg_fod bit, a subsequent
		 * mmreg_rdkl() call cannot see the pages we claim here.
		 *
		 * Also arrange to not be swapped.
		 *
		 * If the read causes an IO error, invalidate the map;
		 * process has already been swkill()'d.  Ok to return
		 * ref to page that didn't really get read, process will
		 * die before doing anything interesting.
		 */

		delta = mmreg_rdkl(mf, pte, bn, &klsize);

		UNLOCK_MEM;
		++p->p_noswap;

		bp = swbuf_alloc(1);
		swapio(bp, p, bn - ctod(delta), pte - delta,
			klsize * ctob(CLSIZE), B_READ|B_PGIN, vp->v_devvp);
		if (bp->b_flags & B_ERROR)
			vp->v_badmap |= MM_IOERR;
		swbuf_free(bp);

		/*
		 * Instrumentation.
		 */

		u.u_ru.ru_majflt++;
		l.cnt.v_pgin++;
		l.cnt.v_pgpgin += klsize * CLSIZE;
		l.cnt.v_exfod += klsize * CLSIZE;

		/*
		 * Fix page table entries and wake up waiters on intransit
		 * pages.
		 *
		 * Only page requested in is validated, and rest of pages
		 * can be ``reclaimed'' from the free-list.
		 *
		 * pte is not valid from above, to coordinate with concurrent
		 * faults in same mfile.
		 */

		LOCK_MEM;

		*(int *) pte |= PG_V;

		for (npte = pte-delta, i = 0; i < klsize; i++, npte += CLSIZE) {
			c = PTETOCMAP(*npte);
			if (c->c_iowait)
				intrans_wake(c);
			c->c_intrans = 0;
			if (npte != pte)
				memfree1(npte);
		}

		UNLOCK_MEM;

		if (klsize > 1 && blocked_sema(&mem_wait))
			vall_sema(&mem_wait);

		/*
		 * All done.  Allow swapping again, and return phys addr
		 * we just worked so hard for.
		 */

		--p->p_noswap;
		break;
	}

	return ((int)PTETOPHYS(*pte));
}

/*
 * mmreg_rdkl()
 *	See about pre-paging for vnode fill-on-demand.
 *
 * Pre-paged FOD pages are marked (cmap[]) that they hold an FOD
 * page.  Realloc before use thus can rebuild the original FOD pte
 * (see lmemall), avoiding swap-space write for not yet used page.
 *
 * Look forward from faulted pte, then back, and locate
 * adjacent FOD pte's that fill from contiguous disk blocks.
 *
 * Caller frees all pages allocated here, keeping reclaim links.
 *
 * Value = # pte's below argument for start.  Returns total count of
 * pages in *pkl.
 *
 * Assumes caller holds memory free-lists locked and guaranteed
 * existence of KLMAX free pages.
 *
 * Could avoid need to hold memory lists locked here if mmreg_ref_page()
 * asserted some other lock on the mfile to insure at most one process
 * at a time is looking at a particular set of pte's here (and arrange
 * that "lmemall" set c_intrans for CMMAP pages).  Since this should
 * be relatively rare, and it can't look beyond KLMAX pages (typically),
 * don't get too fancy.
 */

#ifdef	PERFSTAT
extern	int	fod_kcnt[];
#endif	PERFSTAT

int	fod_kfmax = KLMAX;			/* forward look limit */
int	fod_kbmax = KLMAX;			/* backward look limit */

static
mmreg_rdkl(mf, pte0, bn0, pkl)
	register struct	mfile	*mf;		/* relevant mfile */
	struct	pte	*pte0;			/* pte that is comming in */
	daddr_t		bn0;			/* source block# for pte0 */
	int		*pkl;			/* -> klsize */
{
	register struct pte	*pte;
	register struct	cmap	*c;
	struct	pte	*max_pte;		/* 1st beyond max pte in seg */
	struct	pte	*spte;			/* 1st pre-page pte */
	struct	pte	*lpte;			/* 1st beyond last pre-page pte */
	register daddr_t blkno;
	register int	count;
	int		prot;

	count = 1;				/* for argument pte */

	max_pte = mf->mf_pt + mf->mf_size;

	/*
	 * Look forward...
	 * Must be within range, FOD, not ZFOD, contiguous disk block.
	 * Limit scan by tunable parameters (see fod_klust()).
	 */

	blkno = bn0 + ctod(CLSIZE);
	pte = pte0 + CLSIZE;

	while(	pte < max_pte
	&&	(*(int*)pte & (PG_FZERO|PG_FOD)) == PG_FOD
	&&	PTE_TO_BLKNO(*pte) == blkno
	&&	count < fod_kfmax) {
		/*
		 * Set up to look at next page.
		 */
		pte += CLSIZE;
		blkno += ctod(CLSIZE);
		count++;
	}

	lpte = pte;

	/*
	 * Similarly, look back.
	 */

	blkno = bn0 - ctod(CLSIZE);
	pte = pte0 - CLSIZE;

	while(	pte >= mf->mf_pt
	&&	(*(int*)pte & (PG_FZERO|PG_FOD)) == PG_FOD
	&&	PTE_TO_BLKNO(*pte) == blkno
	&&	count < fod_kbmax) {
		/*
		 * Set up to look at prev page.
		 */
		pte -= CLSIZE;
		blkno -= ctod(CLSIZE);
		count++;
	}

	spte = pte+CLSIZE;

	/*
	 * Allocate memory to the pre-page sites.
	 * Also fill out cmap[] entries to reflect they hold FOD pages.
	 */

	for (pte = spte; pte < lpte; pte += CLSIZE) {
		if (pte == pte0)
			continue;
		/*
		 * Allocate a page, copy protection bits (and leave m-bit off).
		 * Don't set valid; this is pre-paging.
		 */

		blkno = PTE_TO_BLKNO(*pte);
		prot = *(int *)pte & PG_PROT;
		lmemall(pte, CLSIZE, (struct proc *)0, CMMAP,
			&mmap_reg, (u_long)(pte-mf->mf_pt), (u_long) mf);
		*(int*)pte |= prot;

		/*
		 * Set up cmap[] entry to be able to re-build FOD pte's
		 * if page is reallocated before reclaimed (eg, used).
		 * Also set c_intrans to tell other concurrent
		 * processes that the page is on its way in.
		 */

		c = PTETOCMAP(*pte);
		c->c_intrans = 1;			/* tell others */
		c->c_holdfod = 1;			/* holds an FOD page */
		c->c_blkno = blkno;			/* allow rebuild FOD */
	}

	/*
	 * Return results.
	 */

#ifdef	PERFSTAT
	++fod_kcnt[count];
#endif	PERFSTAT

	*pkl = count;
	return (pte0 - spte);
}

/*
 * mmreg_rw()
 *	Read/write IO on mapped vnode.  If overlap, handle from "shared" space.
 *
 * Returns true if match, else false.  Any errors returned in u.u_error.
 *
 * Called from rwip() with locked vnode.  Since vnode locked, there are no
 * sync's, swapout's, swapin's, or unmap's going on for this mfile right now.
 *
 * Handle case of "bad" VTEXT mapping; this can occur from (eg) NFS, since file
 * can change state on the server.  Try to purge mapping in this case; if
 * succeed (it was "tacky"), the vnode is no longer mapped.  If fail, code
 * handles error and unwind (since the map can go bad asynchronously).
 * Hard for (eg) NFS to purge when marked bad since need locked vnode to call
 * xrele()/mmreg_rele(), so check here.  This also handles case of "exec" of
 * stale VTEXT mapping; execve() reads the mapped file before mapping it.
 *
 * This uses single set of mapping pte's allocated in mfinit().  This has
 * lower overhead than dynamic alloc/dealloc, but possible bottleneck if
 * there is much read/write activity to mapped files.  Suspect not much RW
 * to mapped files, however.
 *
 * Could return buf-header mapped into the shared-stuff, but would require
 * 2nd call into here to release the references.  When/if further integrate
 * mapped files, could do this in rwip() directly.
 */

mmreg_rw(vp, lbn, rw, on, n, uio)
	struct	vnode	*vp;
	daddr_t		lbn;			/* file-sys block# */
	enum	uio_rw	rw;			/* UIO_READ | UIO_WRITE */
	int		on;			/* offset in block */
	int		n;			/* # bytes to move */
	struct	uio	*uio;
{
	register struct	mfile *mf;
	register struct	pte *pte;
	register struct	pte *spte;
	register unsigned off;
	register int	i;
	struct	mfile	*fmf;
	u_long		off0;
	u_long		pg;
	int		npg;
	int		oncl;
	u_long		paddr;
	int		error = 0;
	spl_t		s;

	/*
	 * Try to purge a "bad" VTEXT mapping.
	 * If not really mapped, could be VMAPPED|VTEXT flags are stale.
	 */
again:
	if (vp->v_badmap && (vp->v_flag & VTEXT) && mmreg_rele(vp))
		return 0;

	/*
	 * See if intersect.  Use fact that individual mfile's map whole
	 * fs_bsize pieces of file.
	 */

	off = lbn * btop(vp->v_vfsp->vfs_bsize);

	VN_LOCK_MAP_LIST(vp, s);
	if (!VN_MAPPED(vp)) {
		VN_UNLOCK_MAP_LIST(vp, s);
		vp->v_flag &= ~(VMAPPED|VTEXT);
		return 0;
	}
	mf = fmf = &mfile[vp->v_mapx];

	ASSERT_DEBUG(vp == mf->mf_vp, "mmreg_rw: skew");

	do {
		if (MFINTERSECT(mf, off, CLSIZE)) {
			MF_XFR_LOCK(mf, vp, s);
			if (mf->mf_flag & MF_UNMAP_RACE) {
				ASSERT(!MF_BLOCKED(mf), "mmreg_rw: blocked");
				/*
				 *+ On attempting a read/write operation on a
				 *+ file that had been previously mapped, the
				 *+ kernel discovered that another process was
				 *+ blocked waiting for this mfile structure
				 *+ to be released.
				 */
				MF_UNLOCK(mf);
				mfile_dealloc(mf);
				goto again;
			}

			/*
			 * Make sure it's in memory, get exclusive access to
			 * mapping pte's and make references to relevant pages.
			 *
			 * Must hold memory locked for mmreg_ref_page().
			 *
			 * There exits a small probability
			 * of memory deadlock here due to holding mfile in
			 * memory, but can only do one at a time (since
			 * hold mmreg_rwmutex).
			 *
			 * It is possible to hit an unmapped part of the
			 * file -- in this case, return failure.
			 */

			/*
			 * Locked vnode means nothing about mfile's going
			 * away.  Need formal reference, and use
			 * mfile_unref() to release it.  Holding vnode locked
			 * does insure no mmreg_new() calls ==> no concurrent
			 * create of new pte's in the mfile.
			 *
			 * This is essentially the "hit" case of mmreg_new().
			 */
			if (++mf->mf_count == 1) {
				mfile_rmfree(mf);
				if (mf->mf_ccount == 0)
					(void) mfile_lccbump(mf, 1);
			} else
				(void) mfile_lccbump(mf, 1);
			MF_UNLOCK(mf);

			/*
			 * Lock the mmreg_rwpte data structure
			 */
			p_sema(&mmreg_rwmutex, PRSLOCK);

			oncl = clbase(btop(on));
			npg = clrnd(btoc(on+n)) - oncl;

			off0 = (off+oncl) - mf->mf_pos;
			pte = mmreg_rwpte;
			spte = MFOFFTOPTE(mf, off0);

			/*
			 * VTEXT mapped file might not map whole fs block,
			 * so find appropriate npg.  VTEXT mapped vnode
			 * is guaranteed to map starting at 1st page of
			 * block, however.
			 */

			for (; npg > 0; npg -= CLSIZE)
				if (*(int*) &spte[npg-CLSIZE] != PG_INVAL)
					break;
			if (npg == 0) {
				v_sema(&mmreg_rwmutex);
				mfile_unref(mf, 0);
				return (0);
			}

			/*
			 * Get ref's to mapped pages.  If map goes bad,
			 * bail out early (this can set npg=0, but code
			 * below handles this).
			 *
			 * NOTE: once map goes bad, the file can't even
			 * be copied due to this.  Could pass error mask
			 * to mmreg_ref_page(), and allow reads to go thru
			 * the buf cache; expect this problem to be rare,
			 * so keep it simple for now.
			 */

			for (pg=off0, i=0; i < npg; i += CLSIZE, pg += CLSIZE) {
				paddr = mmreg_ref_page(mf, pg);
				if (paddr & MM_BADMAP) {
					npg = i;
					error = (paddr & MM_STALETEXT)
							? ETXTBSY : EIO;
					break;
				}
				copycl(pte, spte);
				*(int *) pte |= PG_V|PG_KW|PG_R|PG_M;
				distcl(pte);
				pte += CLSIZE;
				spte += CLSIZE;
			}

			newptes(mmreg_rwva, (long)npg);

			/*
			 * Move the data.  When done, don't need pte's any more.
			 */

			if (error == 0) {
				on &= CLOFSET;		/* offset in page */
				if (n > (ctob(npg) - on))
					n = ctob(npg) - on;
				error = uiomove(mmreg_rwva+on, n, rw, uio);
			}

			v_sema(&mmreg_rwmutex);

			/*
			 * Release the mapping and return success.
			 * (shortcut calling mmreg_derefpg()).
			 */

			spte = MFOFFTOPTE(mf, off0);
			LOCK_MEM;
			for (i = 0; i < npg; i += CLSIZE, spte += CLSIZE) {
				if (rw == UIO_WRITE)
					*(int*)spte |= PG_M;
				memfree1(spte);
			}
			UNLOCK_MEM;

			mfile_unref(mf, 0);

			if (rw == UIO_WRITE) {
				VOP_MARK(vp, IUPD|ICHG);
			} else {
				VOP_MARK(vp, IACC);
			}

			/*
			 * Use local error variable to avoid problems of
			 * u.u_error references in called procedures.  Global
			 * variables are a problem.
			 */

			if (u.u_error == 0)
				u.u_error = error;
			return (1);
		}

	} while ((mf = mf->mf_next) != fmf);

	VN_UNLOCK_MAP_LIST(vp, s);

	/*
	 * Fall thru ==> no match -- do normal IO.
	 */

	return (0);
}

/*
 * mmreg_umount()
 *	Flush mfile free list of any tacky/sticky maps on a given
 *	virtual file-system.
 *
 * Called during unmount() to flush any stale refs on the file-sys.
 */

mmreg_umount(vfsp)
	struct vfs *vfsp;
{
	register struct mfile *mf;
	register struct vnode *vp;
	register spl_t	s;

	/*
	 * Search mfile free-list backwards and flush relevant maps.
	 * Can stop search when see first fully free mfile -- algorithms
	 * assure no tacky/sticky's before fully free ones.
	 * Since hold free-list locked, can't race with mmreg_alloc() reclaim.
	 */

	LOCK_MFILES(s);
loop:
	for (mf = mfile_free.mf_pfree; mf != &mfile_free; mf = mf->mf_pfree) {
		if ((vp = mf->mf_vp) == NULL)	/* non-tacky/sticky ==> done */
			break;
		if (vp->v_vfsp != vfsp)		/* different file-sys */
			continue;
		if (!VN_TRYLOCKNODE(vp))	/* can't lock ==> fail umount */
			break;
		ASSERT_DEBUG(mf->mf_count == 0, "mmreg_umount: count");
		/*
		 * Got it.  Pull off free-list and release resources
		 * (ala mfile_alloc() tacky re-use), release claim on
		 * vnode as appropriate, and re-start loop.
		 */
		MF_RM_FREE(mf);
		UNLOCK_MFILES(s);
		/*
		 * Ok to call mfile_rele() here without locking mf, since
		 * this is a tacky text, and hold vnode locked (callers attempt
		 * to reclaim text while holding vnode locked).
		 *
		 * Careful to get another vnode reference, since mfile_rele()
		 * may release last mfile reference.
		 */
		VN_HOLD(vp);			/* get another reference */
		mfile_rele(mf, 1);		/* release resources */
		VN_PUT(vp);
		LOCK_MFILES(s);
		MF_INS_HEAD_FREE(mf);
		goto loop;
	}
	UNLOCK_MFILES(s);
}

/*
 * mmreg_flush(dosticky)
 *	Try to delete a tacky/sticky mapped vnode as a means of freeing some
 *	memory and/or vnode.
 *
 * Used as a desperation effort by swapper, and to try to free some vnodes.
 *
 * The only caller with dosticky != 0 is the swapper (sched()).  This
 * argument is mpx'd to also mean the page-table should be freed but
 * the mapping shouldn't be (need to avoid swapper deadlocking trying to
 * update vnode (eg, iinactive() calls iupdat()).
 *
 * Returns true if releases one, else false.
 */

bool_t
mmreg_flush(dosticky)
	bool_t	dosticky;
{
	register struct mfile *mf;
	register struct vnode *vp;
	spl_t	s;

	/*
	 * Lock mfile free-list and search for unused tacky and sticky maps.
	 * List is FIFO: search from front ==> find oldest tacky/sticky first.
	 * Must be careful to not find one already flush'd if "dosticky".
	 */

	LOCK_MFILES(s);

	for (mf = mfile_free.mf_nfree; mf != &mfile_free; mf = mf->mf_nfree) {
		if ((vp = mf->mf_vp) == NULL		/* "free" not useful */
		||  ((vp->v_flag & VSVTX) && !dosticky)	/* ignore sticky's? */
		||  (mf->mf_pt == NULL && dosticky)	/* need pt to free */
		||  !VN_TRYLOCKNODE(vp))		/* want'em locked */
			continue;
		ASSERT_DEBUG(mf->mf_count == 0, "mmreg_flush: count");
		/*
		 * If "dosticky" then only free page-table; can't release
		 * underlying vnode since this could deadlock swapper.
		 * Else fully release the mfile.
		 *
		 * Locked vnode ==> no race with mmreg_alloc().
		 */
		if (dosticky) {
			/*
			 * Ok to call mfile_ccdec() here since this is a
			 * tacky text, and hold vnode locked (attempts to
			 * reclaim text hold vnode locked).
			 */
			UNLOCK_MFILES(s);
			ASSERT(mf->mf_ccount == 1, "mmreg_flush: ccount");
			/*
			 *+ An mfile entry on the free list has a nonzero
			 *+ reference count.
			 */
			mfile_ccdec(mf);
			VN_UNLOCKNODE(vp);
		} else {
			MF_RM_FREE(mf);
			UNLOCK_MFILES(s);
			/*
			 * Ok to call mfile_rele() here without locking mf,
			 * since this is a tacky text, and hold vnode locked
			 * (attempts to reclaim text hold vnode locked).
			 *
			 * Careful to get another vnode reference, since
			 * mfile_rele() may release last mfile reference.
			 */
			VN_HOLD(vp);		/* get another reference */
			mfile_rele(mf, 1);	/* release resources */
			VN_PUT(vp);
			mfile_dealloc(mf);
		}
		return (1);
	}
	UNLOCK_MFILES(s);

	return (0);
}

/*
 * mmreg_rele()
 *	Turn off mapping (VTEXT) in a given vnode if possible.
 *
 * Called when it's desired to open a vnode for writing, and to purge "bad"
 * VTEXT mapping.
 *
 * This can't race with mmreg_alloc() on same vnode since hold locked vnode.
 *
 * Called with locked vnode.
 * Returns true if releases text, else false.
 */

bool_t
mmreg_rele(vp)
	register struct vnode *vp;
{
	register struct mfile *mf;
	spl_t	s;

	/*
	 * While it's still mapped, try to release 1st mapping mfile.
	 * Can't race with mfile_alloc() tacky search since hold locked
	 * vnode and mfile_alloc() does "cp_sema".
	 */

	while (vp->v_flag & VTEXT) {
		VN_LOCK_MAP_LIST(vp, s);
		if (!VN_MAPPED(vp)) {
			VN_UNLOCK_MAP_LIST(vp, s);
			vp->v_flag &= ~(VMAPPED|VTEXT);
			return 1;
		}
		/*
		 * Formally get access to the mfile (need to worry about
		 * races with unmap).
		 */
		mf = &mfile[vp->v_mapx];
		MF_XFR_LOCK(mf, vp, s);
		/*
		 * Might have raced with unmap trying to delete it.
		 */
		if (mf->mf_flag & MF_UNMAP_RACE) {
			ASSERT(!MF_BLOCKED(mf), "mmreg_rele: blocked");
			/*
			 *+ On attempting to release an mfile structure,
			 *+ the kernel discovered a race condition with
			 *+ another process attempting to unmap this mfile.
			 */
			MF_UNLOCK(mf);
			mfile_dealloc(mf);
			continue;
		}
		/*
		 * If count == 0, it can't become non-zero (since hold vnode
		 * locked).  If non-zero, can race with unmap, but that's ok.
		 * Remove mfile from vnode (sort of in-line expanded
		 * mfile_unref()).
		 */
		if (mf->mf_count != 0) {
			MF_UNLOCK(mf);
			return 0;			/* failed */
		}
		mfile_rmfree(mf);			/* off free list */
		mfile_rele(mf, 1);			/* release resources */
		MF_UNLOCK(mf);				/* completely done */
		mfile_dealloc(mf);			/* -> head free list */
	}

	/*
	 * Should only get here if VTEXT wasn't on when called.
	 */

	return 1;
}

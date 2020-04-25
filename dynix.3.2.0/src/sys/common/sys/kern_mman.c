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
static	char	rcsid[] = "$Header: kern_mman.c 2.23 91/03/11 $";
#endif

/*
 * kern_mman.c
 *	Various memory management procedures.
 *
 * Original Berkeley "ifdef notdef"'s around pseudo-implementations
 * of several of the stubs here has been removed in the interest of
 * readability.
 */

/* $Log:	kern_mman.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vnode.h"
#include "../h/file.h"
#include "../h/cmap.h"
#include "../h/conf.h"
#include "../h/vm.h"
#include "../h/seg.h"

#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"

getpagesize()
{
	u.u_r.r_val1 = NBPG * CLSIZE;
}

/*
 * brk(), old version
 *	Adjust size of program data segment.
 *
 * Goes away when/if sbrk() is implemented.
 */

obreak()
{
	struct a {
		char	*nsiz;
	};
	register int n, d;

	/*
	 * set n to new data size
	 * set d to new-old
	 */

	n = btoc(((struct a *)u.u_ap)->nsiz);
	if (n <= 0) {
		/*
		 * Can't make all data space disappear.
		 */
		u.u_error = EINVAL;
		return;
	}

	d = clrnd(n - u.u_dsize);
	if (u.u_error = chksize((u_int)u.u_dsize+d, (u_int)u.u_ssize))
		return;
	if (ctob(u.u_dsize+d) > u.u_rlimit[RLIMIT_DATA].rlim_cur
	||  (d > 0 && !vsalloc(&u.u_dmap, u.u_dsize, (size_t) d))) {
		u.u_error = ENOMEM;
		return;
	}

	if (expand(d, 0, 0, PG_ZFOD)) {
		vexpandRS(d);			/* grow Rset, if appropriate */
		/*
		 * Shrink is probably rare, but maybe more used due to
		 * mapped files -- free any swap space beyond what's needed.
		 */
		if (d < 0)
			vsfree(&u.u_dmap, u.u_dsize, dtoc(maxdmap)-u.u_dsize);
	} else
		u.u_error = ENOMEM;
}

/*
 * grow()
 *	grow the stack to include the SP
 *
 * True return if successful.  The (rare) failure is due to no swap space
 * when trying to swap out to expand page-table.
 */

grow(sp)
	unsigned sp;
{
	register size_t si;

	si = clrnd(btoc((USRSTACK-sp)) - u.u_ssize + SINCR);

	if (sp >= USRSTACK-ctob(u.u_ssize)
	||  ctob(u.u_ssize+si) > u.u_rlimit[RLIMIT_STACK].rlim_cur
	||  chksize((u_int)u.u_dsize, (u_int)u.u_ssize+si) != 0
	||  !vsalloc(&u.u_smap, u.u_ssize, si)
	||  !expand(0, (int)si, 0, PG_ZFOD))
		return (0);
	
	vexpandRS((int)si);			/* grow Rset, if appropriate */
	return (1);
}

/*
 * mmap()
 *	Re-map part of process address space.
 */

mmap()
{
	struct a {
		caddr_t	addr;		/* starting virt-addr */
		int	len;		/* length (bytes) to map */
		int	prot;		/* RO, RW encoding */
		int	share;		/* private/shared modifications */
		int	fd;		/* open file to be mapped */
		u_long	pos;		/* where in file to begin map */
	} *uap = (struct a *)u.u_ap;
	register struct mmap *um;
	register struct pte *pte;
	register size_t	npgcnt;
	register u_long	paddr;
	register int	j;
	struct	proc	*p = u.u_procp;
	struct	file	*fp;
	struct	vnode	*vp;
	struct	mapops	*mapops;
	size_t		pgcnt;
	u_long		pospg;
	u_long		firstpg;
	u_long		lastpg;
	u_long		firstdp;
	u_long		nfirstpg;
	u_long		handle;
	int		map_type;
	int		proto_pte;
	int		share = uap->share;	/* convenience copy of arg */
	int		prot = uap->prot;	/* convenience copy of arg */
	int		max_prot;
	int		ods;
	int		nds;
	bool_t		um_extend = 0;
	extern	int	ummap_max_hole;		/* max u_mmap[] concat hole */

	/*
	 * Must have good alignment of arguments, reasonable address space,
	 * not a vfork child...
	 */

	npgcnt = pgcnt = btop(uap->len);
	nfirstpg = firstpg = btop(uap->addr);
	lastpg = firstpg + npgcnt - 1;
	pospg = btop(uap->pos);

	if (uap->len <= 0
	||  (prot & PROT_RDWR) == 0
	||  (((int)uap->addr | uap->len | uap->pos) & CLOFSET) != 0
	||  lastpg < firstpg
	||  isassv(p, firstpg)
	||  (p->p_flag & SVFORK)) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * Based on share arg, check further and do special case stuff.
	 */

	switch(share) {

	case MAP_PRIVATE:
	case MAP_SHARED:
		/*
		 * Must have valid file-descriptor for vnode,
		 * enough access in the file-descriptor...
		 */

		u.u_error = getvnodefp(uap->fd, &fp);
		if (u.u_error)
			return;
		vp = (struct vnode *)fp->f_data;

		if (share == MAP_PRIVATE) {
			max_prot = PROT_READ;
			if (prot & PROT_WRITE)
				prot &= ~PROT_WRITE;
			else
				share = MAP_SHARED;
		} else
			max_prot = (fp->f_flag&FWRITE) ? PROT_RDWR : PROT_READ;
		if ((prot & max_prot) != (prot & PROT_RDWR)
		||  (fp->f_flag & (FWRITE|FREAD)) == FWRITE) {
			u.u_error = EACCES;
			return;
		}

		/*
		 * Insist on execute access for PROT_EXEC, and !PROT_WRITE.
		 * Execute access avoids issue of mapping RO file and
		 * dis-allowing writes by holding VTEXT mapping.
		 * Mapping function worries about open RW connections, etc.
		 */

		if (prot & PROT_EXEC) {
			if (prot & PROT_WRITE) {
				u.u_error = EINVAL;
				return;
			}
			/*
			 * VOP_ACCESS wants a locked vnode.
			 */
			VN_LOCKNODE(vp);
			u.u_error = VOP_ACCESS(vp, VEXEC, u.u_cred);
			VN_UNLOCKNODE(vp);
			if (u.u_error)
				return;
			max_prot |= PROT_EXEC;
		}

		/*
		 * Ok so far.  If file-descriptor is already mapped, try
		 * to extend previous u_mmap[].  Otherwise, locate an
		 * unused u_mmap[] in this process.
		 *
		 * Could extend for MAP_PRIVATE if check mm_cor, if this case
		 * becomes important.
		 *
		 * This assumes mapping the same thing gives equivalent map.
		 * There is no good way to check this.
		 */

		um = u.u_mmapmax;
		if (share == MAP_SHARED) {
			/*
			 * Check mm_pgcnt, mm_fdidx, and mm_prot (mm_prot
			 * insures don't mix (eg) "exec" and RO map).
			 *
			 * Position in file offset from current u_mmap[] must
			 * agree with offset in process virtual space.  This
			 * allows arbitrary overlap (including completely
			 * replacing previous map).
			 *
			 * Also, insure the map isn't too far away: don't
			 * want to waste underlying map function resources.
			 */
			for (um = u.u_mmap; um < u.u_mmapmax; um++) {
				if (um->mm_pgcnt != 0
				&&  um->mm_fdidx >= 0
				&&  fp == &file[um->mm_fdidx]
				&&  um->mm_prot == max_prot
				&&  (pospg-um->mm_off) == (firstpg-um->mm_1stpg)
				&&  firstpg+npgcnt+ummap_max_hole >= um->mm_1stpg
				&&  um->mm_cor != 1
				&&  firstpg <= um->mm_1stpg+um->mm_size+ummap_max_hole) {
					/*
					 * Got a match!  Set up to map whole
					 * region; later unmap previous.
					 */
					if (pospg > um->mm_off) {
						pospg = um->mm_off;
						firstpg = um->mm_1stpg;
					}
					if (lastpg < um->mm_1stpg + um->mm_size)
						lastpg = um->mm_1stpg + um->mm_size - 1;
					pgcnt = lastpg - firstpg + 1;
					break;
				}
			}
		}
		if (um == u.u_mmapmax) for (um=u.u_mmap; um < u.u_mmapmax; um++)
			if (um->mm_pgcnt == 0)
				break;
		if (um == u.u_mmapmax) {
			if (u.u_mmapmax >= &u.u_mmap[NUMMAP]) {
				u.u_error = EMFILE;
				return;
			}
			um->mm_pgcnt = 0;		/* in case error */
			++u.u_mmapmax;
		}
		break;

	case MAP_ZEROFILL:
		/*
		 * Don't check the fd argument -- unused.
		 */
		if ((prot & PROT_BITS) != PROT_RDWR) {
			u.u_error = EINVAL;
			return;
		}
		break;

	default:
		u.u_error = EINVAL;
		return;
	}

	/*
	 * If don't fit within existing data-space, create new space,
	 * filling holes with invalid pages.  This doesn't allow
	 * overlap into stack (for now).
	 */

	ods = p->p_dsize;
	nds = vtodp(p, lastpg+1);
	firstdp = vtodp(p, nfirstpg);

	if (u.u_error = chksize((u_int)nds, (u_int)u.u_ssize))
		return;
	if (ctob(nds) > u.u_rlimit[RLIMIT_DATA].rlim_cur
	||  (share!=MAP_SHARED && !vsalloc(&u.u_dmap,(size_t)firstdp,npgcnt))) {
		u.u_error = ENOMEM;
		return;
	}
	if (nds > ods) {
		if (!expand(nds-ods, 0, 0, PG_INVAL)) {
			u.u_error = ENOMEM;
			return;
		}
		vexpandRS(nds-ods);
	}

	/*
	 * Based on type of vnode, figure handle and mapping proc
	 * and see about the mapping.
	 *
	 * Note that ask for max possible access, since user may
	 * later mprotect() a RO page into a RW page.
	 */

	if (share != MAP_ZEROFILL) {
		mapops = vnode_mmap[(int)vp->v_type];
		VN_LOCKNODE(vp);		/* insure consistent state */
		map_type = (*mapops->map_new)(vp, pospg, pgcnt, max_prot, &handle);
		VN_UNLOCKNODE(vp);

		if (map_type > 0) {				/* error? */
			if (nds > ods)				/* grew above */
				(void) expand(ods-nds, 0, 0, 0);/* shrink */
			u.u_error = map_type;
			return;
		} else if (map_type != MM_PAGED && share == MAP_PRIVATE)
			share = MAP_SHARED;		/* for now */
	}

	/*
	 * Loose old address-space.
	 *
	 * Dis-allow swaps starting now, since u_mmap[] may be termprarily
	 * inconsistent (and mapping function procedures may allow swapping).
	 */

	++p->p_noswap;

	pte = dptopte(p, firstdp);
	p->p_rssize -= vmemfree(pte, (int)npgcnt);

	/*
	 * MAP_ZEROFILL is special-case -- just fill in PT.
	 */

	if (share == MAP_ZEROFILL) {
		ptefill(pte, PG_ZFOD, npgcnt);
		l.cnt.v_nzfod += npgcnt;
		--p->p_noswap;
		return;
	}

	/*
	 * If the u_mmap[] is being extended, MM_UNMAP old version.
	 * If totally overlapped, this went away already in vmemfree().
	 * Note: `handle' may change (eg, mmreg_alloc()).
	 */

	if (um->mm_pgcnt != 0) {
		ASSERT(	um->mm_paged ? map_type == MM_PAGED
		:	(um->mm_noio ? map_type == MM_PHYS
		:	map_type == MM_NPMEM), "mmap: skew");
		/*
		 *+ While unmapping a page, the kernel discovered that
		 *+ the type of the mapped page is inconsistent with
		 *+ other attributes of the page description.
		 */
		(*mapops->map_unmap)(um->mm_handle, um->mm_off,
				um->mm_size, um->mm_prot);
		um_extend = 1;
	}

	/*
	 * Set up mmap structure.
	 *
	 * Note that mm_prot is set for maximum possible file-descriptor access,
	 * not requested access, to anticipate mprotect() function.
	 * Dis-allowed write-only file above (inconsistent with MMU capability).
	 *
	 * MM_PAGED and MM_NPMEM allow IO services, MM_PHYS doesn't.
	 */

	um->mm_off = pospg;
	um->mm_1stpg = firstpg;
	um->mm_size = pgcnt;
	um->mm_pgcnt += npgcnt;				/* += for extending */
	um->mm_handle = handle;
	um->mm_ops = mapops;
	um->mm_fdidx = (fp - &file[0]);			/* file-table index */
	if (!um_extend)					/* need new ref if */
		FDBUMP(fp);				/* not extending */
	um->mm_paged = (map_type == MM_PAGED);
	um->mm_noio = (map_type == MM_PHYS);
	um->mm_prot = max_prot;
	um->mm_lastfd = 0;
	um->mm_cor = (share == MAP_PRIVATE);
	um->mm_text = 0;

	/*
	 * If shared map, don't need swap space under these addresses any more.
	 * Special case: if mapping thru end of data-space, make sure
	 * to delete last chunk of swap.
	 */

	if (share == MAP_SHARED) {
		if (firstdp + npgcnt == p->p_dsize)
			vsfree(&u.u_dmap, (size_t)firstdp, (size_t)(dtoc(maxdmap)-firstdp));
		else
			vsfree(&u.u_dmap, (size_t)firstdp, npgcnt);
	}

	/*
	 * Re-paint address space with new mapping.
	 */

	proto_pte = MAPXTOPTE(um-u.u_mmap)
		  | (((prot & PROT_WRITE) || share == MAP_PRIVATE) ? RW : RO)
		  | PG_R;

	if (map_type == MM_PAGED) {
		/*
		 * Set up paged map -- fill with (invalid) proto_pte,
		 * to fault in pages on demand.
		 */
#ifdef	MMU_MBUG
		proto_pte |= PG_M;			/* redundant */
#endif	MMU_MBUG
		ptefill(pte, proto_pte, npgcnt);
	} else {
		/*
		 * Phys-mapped address-space or non-paged memory:
		 * call driver for each kernel-size page.
		 */
		proto_pte |= PG_M|PG_V;
		firstpg = btop(uap->pos);

		for (; npgcnt != 0; npgcnt -= CLSIZE, firstpg += CLSIZE) {
			paddr = (*mapops->map_refpg)(handle, firstpg);
			for (j = 0; j < CLSIZE; j++, pte++, paddr += NBPG)
				*(int*)pte = PHYSTOPTE(paddr) | proto_pte;
			if (npgcnt < CLSIZE)
				break;
		}

		if (um->mm_noio && !um_extend)
			++u.u_pmapcnt;
	}
	--p->p_noswap;					/* ok to swap now */

	/*
	 * Flush MMU TLB, since process mapping changed a bunch.
	 */

	FLUSH_USER_TLB(p->p_ptb1);
}

/*
 * munmap()
 *	Release some address-space and replace with invalid pages.
 */

munmap()
{
	register struct a {
		caddr_t	addr;		/* starting virt-addr */
		int	len;		/* length (bytes) to unmap */
	} *uap = (struct a *)u.u_ap;
	register struct	proc *p = u.u_procp;
	register struct pte *pte;
	register size_t	pgcnt;
	register unsigned firstpg;
	unsigned	lastpg;

	/*
	 * Must have good alignment of arguments and valid address space.
	 *
	 * For now, insist addresses be entirely in data space.
	 * This restriction should be relaxed when mmap() is more general.
	 */

	pgcnt = btop(uap->len);
	firstpg = btop(uap->addr);
	lastpg = firstpg + pgcnt - 1;

	if (uap->len <= 0 || (((int)uap->addr | uap->len) & CLOFSET)
	||  !isadsv(p, firstpg) || !isadsv(p, lastpg)) {
		u.u_error = EINVAL;
		return;
	}

	/*
	 * No argument errors.
	 * Loose old address-space, and re-fill out with zero pte's.
	 *
	 * Release of u_mmap[] entries is automatic in vmemfree(),
	 * when mm_pgcnt reaches zero.
	 *
	 * Since vmemfree() cleans valid and reclaimable state in
	 * pte's, is ok to allow swap.  Won't swap anyhow, since
	 * vmemfree() dis-allows.
	 */

	pte = dvtopte(p, firstpg);
	p->p_rssize -= vmemfree(pte, (int)pgcnt);

	ptefill(pte, PG_INVAL, pgcnt);

	/*
	 * Flush MMU TLB, since process mapping changed a bunch.
	 */

	FLUSH_USER_TLB(p->p_ptb1);
}

/*
 * munmapum()
 *	Force unmap a given u_mmap[] entry.
 *
 * Called by core() to unmap phys-mapped stuff.
 */

munmapum(um)
	register struct	mmap	*um;
{
	struct	proc *p = u.u_procp;
	register struct	pte *pte;
	register int	mapx;

	ASSERT_DEBUG(um->mm_pgcnt != 0, "munmapum: mm_pgcnt");

	/*
	 * Map is going away.  Mark if fd has no other references.
	 */

	if (um->mm_fdidx < 0 || file[um->mm_fdidx].f_count == 1)
		um->mm_lastfd = 1;

	/*
	 * Run thru page-table and zap all references.
	 *
	 * Note: any private reclaimable pte's are !PTEMAPPED() and don't look
	 * mapped even if being concurrently reallocated.
	 */

	mapx = um - u.u_mmap;
	for (pte = dvtopte(p, um->mm_1stpg); um->mm_pgcnt != 0; pte += CLSIZE) {
		if (!pte->pg_fod && PTEMAPPED(*pte) && PTETOMAPX(*pte)==mapx) {
			p->p_rssize -= vmmapfree(p, pte, 1);
			zapcl(pte, ~PG_INVAL);	/* should abstract better */
		}
	}

	FLUSH_USER_TLB(p->p_ptb1);
}

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
static	char	rcsid[] = "$Header: vm_subr.c 2.12 91/03/05 $";
#endif

/*
 * vm_subr.c
 *	Various VM related subroutines.
 */

/* $Log:	vm_subr.c,v $
 */

#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/vm.h"
#include "../h/mutex.h"
#include "../h/proc.h"
#include "../h/cmap.h"
#include "../h/vnode.h"
#include "../h/buf.h"
#include "../h/vfs.h"
#include "../h/file.h"
#include "../ufs/inode.h"
#include "../ufs/fs.h"

#include "../machine/plocal.h"
#include "../machine/mftpr.h"
#include "../machine/mmu.h"

/*
 * vmaccess()
 *	Validate the kernel map for size ptes which
 *	start at ppte in the Sysmap, and which map
 *	kernel virtual addresses starting with vaddr.
 *
 * We set Ref and Mod bits to avoid MMU bugs and since this doesn't
 * hurt here.  In fact, setting these bits is a performance gain since
 * the MMU doesn't have to re-write the pte's.
 *
 * Assumes caller insures exclusive access to target pte's.
 */

/*ARGSUSED*/
vmaccess(ppte, vaddr, size)
	register struct pte *ppte;
	register caddr_t vaddr;
	register int size;
{
	while (size != 0) {
		*(int*)ppte |= PG_V|PG_KW|PG_R|PG_M;
		ppte++;
		--size;
	}
#ifdef	lint
	lint_ref_int((int)vaddr);		/* compensate for lint bug */
#endif	lint
}

/*
 * Convert a virtual page 
 * number to a pte address.
 */
struct pte *
vtopte(p, v)
	register struct proc *p;
	register unsigned v;
{

	if (isadsv(p, v))
		return (dvtopte(p, v));
	else
		return (svtopte(p, v));
}

/*
 * vinifod()
 *	Init page-tables for fill-on-demand.
 *
 * Initialize the page tables for paging from an inode,
 * by scouring up the indirect blocks in order.
 * Corresponding area of memory should have been vmemfree()d
 * first or just created.
 *
 * Prot argument of zero indicates call by mapped file support;
 * dozfod decides if ok to create space for ZFOD pages.
 * Algorithm invalidates found buf-headers in this case as well.
 *
 * This algorithm could be speeded up for large files by expanding the
 * bmap's internally (ala itrunc()).
 *
 * Returns 1 for success, else 0.
 */

/*VARARGS5*/
vinifod(pte, vp, bfirst, count, prot, dozfod)
	register struct pte *pte;		/* starting pte */
	register struct vnode *vp;		/* vnode to fod from */
	daddr_t		bfirst;			/* cluster # */
	size_t		count;			/* # HW pages to map */
	int		prot;			/* RO or RW: for init'd pte's */
	bool_t		dozfod;			/* alloc space for zfod's? */
{
	register int	i;
	daddr_t 	bn;
	struct	buf	*bp;
	int		blast;
	int		bsize = vp->v_vfsp->vfs_bsize;
	int		nclpbsize = bsize / CLBYTES;

	blast = bfirst + howmany(count, CLSIZE);

	ASSERT_DEBUG(nclpbsize > 0, "vinifod: nclpbsize");

	prot |= PG_FOD;					/* want fod, too */

	while (bfirst < blast) {
		i = bfirst % nclpbsize;
		VOP_BMAP(vp, bfirst/nclpbsize, NULLVP, &bn, B_READ, 0);
		/*
		 * If no current space and can set up ZFOD space,
		 * do for whole fs block.
		 */
		if ((bn < 0 || u.u_error) && prot == PG_FOD) {
			if (!dozfod || u.u_error)
				return(0);
			VOP_BMAP(vp, bfirst/nclpbsize, NULLVP, &bn,
							B_WRITE|B_NOCLR, bsize);
			if (bn < 0 || u.u_error)
				return(0);
			blkinval(vp->v_devvp, bn, (long)bsize);
			ASSERT(i==0, "vinifod: ZFOD");
			/*
			 *+ When the kernel initializes a process's
			 *+ page tables to fill on demand from a file,
			 *+ if the block being mapped doesn't exist
			 *+ and that memory region is marked as
			 *+ zero-fill-on-demand, a data block is
			 *+ allocated and attached to the file.
			 *+ Because the filesystem block size is a
			 *+ multiple of the page cluster size, this
			 *+ allocation should be done only for the
			 *+ first cluster that lives in the data
			 *+ block.  The kernel found that a data block
			 *+ was being allocated for a cluster that
			 *+ wasn't the first in that block.
			 */
			while (i++ < nclpbsize) {
				*(int*)pte = BN_TO_PGBLKNO(bn) | PG_FOD|PG_FZERO;
				distcl(pte);
				pte += CLSIZE;
				bn += ctod(CLSIZE);
				l.cnt.v_nzfod += CLSIZE;
				if (++bfirst == blast)
					return(1);
			}
			continue;
		}
		/*
		 * If setting up for mapped file, must invalidate buf-cache
		 * in case try to read/write after map goes away.
		 */
		if (prot == PG_FOD && (bp = baddr(vp->v_devvp, bn, bsize))) {
			ASSERT((bp->b_flags & B_DELWRI) == 0, "vinifod: bp");
			/*
			 *+ When the kernel maps a file into memory,
			 *+ all disk buffers are written out, if
			 *+ necessary, and then invalidated.  While
			 *+ marking the buffers invalid, the kernel
			 *+ found a buffer with the dirty bit set.
			 */
			bp->b_flags |= B_INVAL;
			brelse(bp);
		}
		/*
		 * Fill out pte's for remainder of fs_bsize chunk.
		 * Already handled case of new ZFOD space.
		 */
		for (; i < nclpbsize; i++) {
			if (u.u_error || bn < 0)	/* no ZFOD setup here */
				return(0);
			*(int*)pte = BN_TO_PGBLKNO(bn+btodb(i*CLBYTES)) | prot;
			l.cnt.v_nexfod += CLSIZE;
			/*
			 * distcl() is sufficient, since decisions
			 * always use 1st pte of cluster.
			 */
			distcl(pte);
			pte += CLSIZE;
			bfirst++;
			if (bfirst == blast)
				break;
		}
	}
	return(1);
}

/*
 * chgprot()
 *	Change protection codes of pages in text segment.
 *
 * If this is 1st call in a given process, the text segment is transformed
 * into a copy-on-ref map.  This way any new references to text pages
 * become private to the process and changes are not seen by others
 * executing the text.  Any changes can be seen on fork, however.
 *
 * Note that "text segment" of process may be mmap'd over, thus can't
 * assume "text" addresses are really text, or that they're RO.
 *
 * Returns old protection in the page (allows clean restore).
 */

chgprot(addr, tprot)
	caddr_t	addr;
	int	tprot;
{
	register struct proc *p = u.u_procp;
	register struct pte *pte;
	register struct mmap *um;
	unsigned v;
	int	oldprot;

	v = clbase(btop(addr));
	if (v < LOWPAGES || v >= u.u_tsize) {
		u.u_error = EFAULT;
		return (0);
	}
	/*
	 * Look at pte...  If mapped, insist on it being a "text" map,
	 * and see about making the text copy-on-ref.
	 * Not mapped ==> ok to change.  If ever do mprotect(), this
	 * allows writing on otherwise private RO pages that aren't
	 * text (no prob).  Only FOD's are ZFOD, which are RW.
	 */
	pte = dvtopte(p, v);
	ASSERT(pte->pg_fod == 0, "chgprot: FOD");
	/*
	 *+ On trying to change the protection attributes of text pages
	 *+ (such as on ptrace), the kernel encountered a page that was marked
	 *+ fill-on-demand.
	 */
	if (PTEMAPPED(*pte)) {
		um = &u.u_mmap[PTETOMAPX(*pte)];
		if (!um->mm_text) {
			u.u_error = EFAULT;
			return (0);
		}
		/*
		 * If 1st time thru, make it legit to copy-on-ref.
		 * Make sure there's swap space under it and set
		 * copy-on-ref bit.  Any existing refs are RO and
		 * don't need to be replaced now (sort of dynamic COR).
		 */
		if (!um->mm_cor) {
			if (!vsalloc(&u.u_dmap, LOWPAGES, (size_t) u.u_tsize-LOWPAGES)) {
				u.u_error = ENOMEM;
				return (0);
			}
			um->mm_cor = 1;
		}
		/*
		 * Make sure page is invalid so COR gets private copy.
		 * Note that if vfork() child, the act of getting private
		 * page ==> delete mapped page ==> u_mmapdel gets set.  Thus
		 * no need to set it here.
		 */
		if (pte->pg_v) {
			(void) vmmapfree(p, pte, 0);
			--p->p_rssize;
		}
	} else if (*(int *)pte == PG_INVAL) {
		u.u_error = EFAULT;
		return (0);
	}
	oldprot = *(int *)pte & PG_PROT;
	*(int *)pte &= ~PG_PROT;
	*(int *)pte |= tprot;
	distcl(pte);
	FLUSH_USER_TLB(p->p_ptb1);		/* in case in TLB */
	return (oldprot);
}

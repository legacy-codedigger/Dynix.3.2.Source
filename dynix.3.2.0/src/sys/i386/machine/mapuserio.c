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
static	char	rcsid[] = "$Header: mapuserio.c 2.7 87/07/30 $";
#endif

/*
 * mapuserio.c
 *	C-versions machine/userio.s routines used when
 *	calling process has physical maps.
 *
 * These could be done more efficiently in asm (add to userio.s),
 * but accessing u_mmap[] is a pain in asm.  Since these should be
 * low runners, no problem.
 */

/* $Log:	mapuserio.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/seg.h"

#include "../machine/pte.h"
#include "../machine/plocal.h"
#include "../machine/mmu.h"

/*
 * _useracc()
 *	Check ability to access user space when process has phys-maps.
 *
 * Called from useracc() when process has phys-maps, since need to check
 * for mapped pte's and check against u_mmap[].mm_noio.
 *
 * Done in C to keep it simpler, suspect low runner.
 *
 * Uses old technique of finding pte's (not self-map) to avoid unwanted faults.
 *
 * Returns 1 for access ok, else 0.
 */

_useracc(base, count, for_read)
	caddr_t  base;
	register int count;
	bool_t	for_read;			/* 0 ==> write, else read */
{
	register struct pte *pte;
	register struct proc *p;
	struct	pte *lim;
	unsigned vpn;
	int	prot;

	p = u.u_procp;

	/*
	 * Check address space, much like copyin()/copyout().
	 * Phys-mapped pages are a no-no.
	 *
	 * First figure flavor of pte, and the limits.
	 */

	prot = (for_read ? RO : RW);

	vpn = clbase(btop(base));
	if (isassv(p, vpn)) {
		pte = svtopte(p, vpn);
		lim = sptopte(p, 0);
	} else if (isadsv(p, vpn)) {
		pte = dvtopte(p, vpn);
		lim = dptopte(p, p->p_dsize-1);
	} else
		return(0);

	count += (int)base & CLOFSET;		/* in case non-aligned */
	ASSERT_DEBUG(((unsigned)count) <= 128*1024, "useracc: count > 128K");
	while (count > 0) {

		/*
		 * If out of segment or not enough access, too bad.
		 */

		if (pte > lim || (*(int*)pte & PG_PROT) < prot)
			return(0);

		/*
		 * Dis-allow phys-mapped address space.
		 */

		if (!pte->pg_fod
		&&  PTEMAPPED(*pte)
		&&  u.u_mmap[PTETOMAPX(*pte)].mm_noio)
			return(0);

		count -= CLBYTES;
		pte += CLSIZE;
	}

	return(1);
}

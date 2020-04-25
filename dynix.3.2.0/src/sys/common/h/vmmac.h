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

/*
 * $Header: vmmac.h 2.4 87/03/11 $
 *
 * vmmac.h
 *	Virtual memory related conversion macros
 *
 * p_p0br from the original is replaced by p_ul2pt, for more consistency
 * with the implementation.
 */

/* $Log:	vmmac.h,v $
 */

/* Core clicks to number of pages of page tables needed to map that much */
#define	ctopt(x)	(((x)+NPTEPG-1)/NPTEPG)

/* Virtual page numbers to data|stack segment page numbers and back */
#define	vtodp(p, v)	((int)(v))
#define	vtosp(p, v)	((int)(btop(USRSTACK) - 1 - (v)))
#define	dptov(p, i)	((unsigned)(i))
#define	sptov(p, i)	((unsigned)(btop(USRSTACK) - 1 - (i)))

/* Tell whether virtual page numbers are in data or stack segment */
#define	isassv(p, v)	((v) >= btop(USRSTACK) - (p)->p_ssize)
#define	isadsv(p, v)	((v) < (p)->p_dsize)

/* Data|stack pte's to segment page numbers and back */
#define	ptetodp(p, pte)		((pte) - (p)->p_ul2pt)
#define	ptetosp(p, pte)		(((p)->p_pttop - 1) - (pte))

#define	dptopte(p, i)		((p)->p_ul2pt + (i))
#define	sptopte(p, i)		(((p)->p_pttop - 1) - (i))

/* dptetov(p,pte) == dptov(p,ptetodp(p,pte)), but avoids redundant arithmetic */
#define	dptetov(p, pte)		((pte) - (p)->p_ul2pt)

/* Xvtopte(p,i) == Xptopte(p,vtoXp(p,i)), but avoids redundant arithmetic */
#define	dvtopte(p, v)		((p)->p_ul2pt + (int)(v))
#define	svtopte(p, v)		((p)->p_pttop - ((int)(btop(USRSTACK) - (v))))

/* Bytes to pages without rounding, and back */
#define	btop(x)		(((unsigned)(x)) >> PGSHIFT)
#define	ptob(x)		((caddr_t)((x) << PGSHIFT))

/*
 * Turn kernel virtual addresses for user page-table pages into
 * kernel map indices (and back).
 */
#define	kmxtob(a)	(usrpt + (a) * NPTEPG)
#define	btokmx(b)	(((b) - usrpt) / NPTEPG)

/*
 * VALLOC_RS_SLOT() insures there is space in the resident-set of a process
 * for another page, calling vallocRSslot() to make room if needed.
 */

#define	VALLOC_RS_SLOT(p) \
	if ((p)->p_rssize >= (p)->p_rscurr) vallocRSslot(p)

/*
 * Include machine dependent macros.
 */

#ifdef	KERNEL
#include "../machine/vmmac.h"
#else
#include <machine/vmmac.h>
#endif	KERNEL

/* Average new into old with aging factor time */
#define	ave(smooth, cnt, time) \
	smooth = ((time - 1) * (smooth) + (cnt)) / (time)

/*
 * Page clustering macros.
 * 
 * anycl(pte,fld)		does any pte in the cluster has fld set?
 * zapcl(pte,mask)		zaps mask bits cluster
 * zap_pfnum(pte)		zaps pfnum, reserved (& mapx) fields in cluster
 * distcl(pte)			distribute state bits to cluster; currently
 *				copies low 8-bits.
 * copycl(tpte,spte)		copy cluster of ptes
 * newuptes(vaddr)		zaps TLB of user virt-addrs for page starting
 *				at vaddr.
 *
 * In all cases, pte must be the low pte in the cluster, even if
 * the segment grows backwards (e.g. the stack).
 *
 * Note: using "fields" of pte as done here may be inefficient; maybe
 * better to use bit-masks and/or out-of line asm for performance,
 * especially for larger CLSIZE's.
 *
 * The use of the (int*) cast in copycl() is to get the compiler to generate
 * better code.
 *
 * Careful using bit-fields (especially pg_pfnum); ns320xx does 32-bit RMW
 * starting in byte holding 1st bit.  Thus use zapcl() to clear pg_pfnum.
 */

#if CLSIZE==1
#define	anycl(pte,fld)	((pte)->fld)
#define	distcl(pte)
#define	zapcl(pte,mask) { *(int*)(pte) &= ~(mask); }
#define	copycl(tpte,spte) { *(tpte) = *(spte); }
#define	newuptes(vaddr)   mtmr(eia, EIA_USER|((unsigned)(vaddr)))
#endif

#if CLSIZE==2
#define	anycl(pte,fld)	((pte)->fld || (((pte)+1)->fld))
#define	distcl(pte) 	*(char*)((pte)+1) = *(char*)((pte)+0)
#define	zapcl(pte,mask) { \
		((int*)(pte))[0] &= ~(mask); \
		((int*)(pte))[1] &= ~(mask); \
	}
#define	copycl(tpte,spte) { \
		((int*)(tpte))[0] = ((int*)(spte))[0]; \
		((int*)(tpte))[1] = ((int*)(spte))[1]; \
	}
#define	newuptes(vaddr)	{ \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+1*NBPG); \
	}
#endif

#if CLSIZE==4
#define	anycl(pte,fld) \
    ((pte)->fld || (((pte)+1)->fld) || (((pte)+2)->fld) || (((pte)+3)->fld))
#define	distcl(pte) \
		*(char*)((pte)+3) = *(char*)((pte)+2) = \
		*(char*)((pte)+1) = *(char*)((pte)+0)
#define	zapcl(pte,mask) { \
		((int*)(pte))[0] &= ~(mask); \
		((int*)(pte))[1] &= ~(mask); \
		((int*)(pte))[2] &= ~(mask); \
		((int*)(pte))[3] &= ~(mask); \
	}
#define	copycl(tpte,spte) { \
		((int*)(tpte))[0] = ((int*)(spte))[0]; \
		((int*)(tpte))[1] = ((int*)(spte))[1]; \
		((int*)(tpte))[2] = ((int*)(spte))[2]; \
		((int*)(tpte))[3] = ((int*)(spte))[3]; \
	}
#define	newuptes(vaddr)	{ \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+1*NBPG); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+2*NBPG); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+3*NBPG); \
	}
#endif

#if CLSIZE==8
#define	anycl(pte,fld) \
    (((pte)+0)->fld || (((pte)+1)->fld) || (((pte)+2)->fld) || (((pte)+3)->fld) \
     ((pte)+4)->fld || (((pte)+5)->fld) || (((pte)+6)->fld) || (((pte)+7)->fld))
#define	distcl(pte) \
		*(char*)((pte)+7) = *(char*)((pte)+6) = \
		*(char*)((pte)+5) = *(char*)((pte)+4) = \
		*(char*)((pte)+3) = *(char*)((pte)+2) = \
		*(char*)((pte)+1) = *(char*)((pte)+0)
#define	zapcl(pte,mask) { \
		((int*)(pte))[0] &= ~(mask); \
		((int*)(pte))[1] &= ~(mask); \
		((int*)(pte))[2] &= ~(mask); \
		((int*)(pte))[3] &= ~(mask); \
		((int*)(pte))[4] &= ~(mask); \
		((int*)(pte))[5] &= ~(mask); \
		((int*)(pte))[6] &= ~(mask); \
		((int*)(pte))[7] &= ~(mask); \
	}
#define	copycl(tpte,spte) { \
		((int*)(tpte))[0] = ((int*)(spte))[0]; \
		((int*)(tpte))[1] = ((int*)(spte))[1]; \
		((int*)(tpte))[2] = ((int*)(spte))[2]; \
		((int*)(tpte))[3] = ((int*)(spte))[3]; \
		((int*)(tpte))[4] = ((int*)(spte))[4]; \
		((int*)(tpte))[5] = ((int*)(spte))[5]; \
		((int*)(tpte))[6] = ((int*)(spte))[6]; \
		((int*)(tpte))[7] = ((int*)(spte))[7]; \
	}
#define	newuptes(vaddr)	{ \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+1*NBPG); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+2*NBPG); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+3*NBPG); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+4*NBPG); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+5*NBPG); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+6*NBPG); \
		mtmr(eia, EIA_USER|((unsigned)(vaddr))+7*NBPG); \
	}
#endif

#define	zap_pfnum(pte)	zapcl(pte,PG_RSVD|PG_PFNUM)

#ifdef	i386
/*
 * Intel 80386 doesn't support flushing single TLB entry.
 */
#undef	newuptes
#endif	i386

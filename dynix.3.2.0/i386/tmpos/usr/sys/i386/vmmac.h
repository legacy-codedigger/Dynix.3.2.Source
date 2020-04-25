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

/*
 * $Header: vmmac.h 2.3 1991/05/13 15:11:07 $
 *
 * vmmac.h
 *	Machine dependent virtual memory related macros.
 *
 * i386 version.
 */

/* $Log: vmmac.h,v $
 *
 */

/*
 * Size of user level 2 page table from text+data, and stack sizes.
 * Note that stack and data are "separate" due to two-level PT structure.
 *
 * No special case to allow stack and data pages to meet, forming "full"
 * address space, unless they meet on L2PT page boundary.  Address space
 * is big enough that don't need the additional complication.  Size checks
 * will bump case where stack+data meet in middle of level-2 page.
 * The "clrnd" in SZL2PT avoids complications if CLSIZE>1 in ptexpand().
 *
 * Per-process shared kernel+user page-table is allocated with user
 * page-table, concatenated at end.
 *
 * Uarea is mapped at top of "user stack", for fast context switch (no
 * reprogramming pte's necessary for this).  Macros take care to avoid
 * accessing Uarea pte's (ie, sptopte(p,0) is a user stack page, not
 * Uarea page).
 *
 * The kernel stack is mapped above the struct user and grows towards it.
 */

#define	UBTOUSER(ub)	(struct user *)((char *)(ub))
#define	USERTOUB(ua)	((char *)(ua))

#define	SZL2PT(tds,ss) (clrnd(ctopt(tds)) + clrnd(ctopt((ss)+UPAGES)))
#define	SZPT(p) 	clrnd(SZL2PT((p)->p_dsize,(p)->p_ssize) + KL1PT_PAGES)

/*
 * SZSWPT() gives size (pages) of swap-space for page-table.
 * This size inludes the Uarea mapping pte, but does not include
 * the shared user+kernel page-table.  This allows
 * an 'easy' page-table swap to be one IO operation.
 * Note that this agrees closely with PTTOP().
 */

#define	SZSWPT(p)	(SZPT(p) - KL1PT_PAGES)

/*
 * Macros to manipulate user page-table representation.
 *
 * UL2PT	determines kernel vaddr for user PT, given Usrptmap[] index.
 * PTBASE	start of user page-table in kernel space.
 * PTTOP	determines top of page-table: 1st pte after last user stack pte.
 * UAREAPTES	kernel address of Uarea mapping ptes in user page-table.
 * L1PT		locates shared kernel+user page-table.
 * UL1PT	locates subset of L1PT that serves as user level-1 page-table.
 * PHYSUL1PT	physical address of shared page-table; used in context switch;
 *		'U' in name is vestige from ns32000 code, for portability.
 * VFILL_MAXL1	max L1 PT index from UL1PT(p), used in vfill_ul1pt().
 * VFILL_MAXL2	max L2 PT index, used in vfill_ul1pt().
 */

#define	UL2PT(idx)	kmxtob(idx)
#define	PTBASE(p)	((p)->p_ul2pt)
#define	PTTOP(p)	(PTBASE(p) + (((p)->p_szpt-KL1PT_PAGES)*NPTEPG-UPAGES))
#define	UAREAPTES(p)	((p)->p_pttop)
#define	L1PT(p)		((p)->p_pttop + UPAGES)
#define	UL1PT(p)	(L1PT(p) + L1IDX(VA_USER))
#define	PHYSUL1PT(p)	PTEPF(Usrptmap[btokmx(L1PT(p))])
#define	VFILL_MAXL1(p)	L1IDX(USER_SPACE)
#define	VFILL_MAXL2(p)	btokmx(L1PT(p))

/*
 * FLUSH_USER_TLB flushes user-TLB in a portable way.
 */

#define	FLUSH_USER_TLB(paddr)	WRITE_PTROOT(paddr)

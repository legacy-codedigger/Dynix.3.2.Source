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
 * $Header: pte.h 2.10 90/11/08 $
 *
 * pte.h
 *	Intel 80386 Page-Table Entry.
 *
 * There are two major kinds of pte's: those which have ever existed (and are
 * thus either now in core or on the swap device), and those which have
 * never existed, but which will be filled on demand at first reference.
 * There is a structure describing each.
 */

/* $Log:	pte.h,v $
 */

/*
 * Normal flavor pte.
 */

struct	pte	{
unsigned int
		pg_v:	  1,		/* valid bit */
		pg_prot:  2,		/* access control */
			: 2,		/* reserved by Intel */
		pg_ref:   1,		/* referenced bit */
		pg_m:	  1,		/* hardware maintained modified bit */
		pg_fod:	  1,		/* is fill on demand (==0 here) */
		        : 4,		/* reserved */
		pg_pfnum:20;		/* page frame number or 0 */
};

/*
 * Fill-On-Demand pte.  Given for form, use of bit-fields is avoided in
 * the implementation since compilers generate incorrect or inefficient code.
 *
 * BN_TO_PGBLKNO() sets up block number for OR-ing into pte.
 * PTE_TO_BLKNO() retrieves pg_blkno field from a pte.
 */

#ifdef	notdef
struct	fpte	{
unsigned int
		pg_v:	  1,		/* valid bit (==0 here) */
		pg_prot:  2,		/* access control */
			: 2,		/* reserved by Intel */
		pg_ref:   1,		/* referenced bit */
		pg_m:	  1,		/* hardware maintained modified bit */
		pg_fod:	  1,		/* is fill on demand (==1 here) */
		pg_fzero: 1,		/* is zero-fill on demand */
		pg_blkno:23;		/* block number on source device */
};
#endif	notdef

#define	BN_TO_PGBLKNO(bn)	((unsigned)(bn) << 9)
#define	PTE_TO_BLKNO(pte)	(*(unsigned *)(&(pte)) >> 9)

#ifdef	notdef
/*
 * Mapped pte.  Given for its form; not actually used in the implementation.
 *
 * pg_mapx encodes mapped index+1; non-zero ==> mapped, if pg_fod==0.
 * Thus could have up to 15 mmap'd pieces in a process.
 * Code must insure pg_fod==0 before looking at pg_mapx.
 * pg_mapx is zero for "fill from swap" pte.
 *
 * Could use pg_fod==0 && pg_fzero==1 ==> mapped, and have 3-bit mapx,
 * but this limits to 8 maps per process.
 *
 * Unified VM/mmap() could be more efficient, avoiding the +1 encoding.
 */

struct	mpte	{
unsigned int
		pg_v:	  1,		/* valid bit (==0 here) */
		pg_prot:  2,		/* access control */
			: 2,		/* reserved by Intel */
		pg_ref:   1,		/* referenced bit */
		pg_m:	  1,		/* hardware maintained modified bit */
		pg_fod:	  1,		/* == 0 for mapped pte */
		pg_mapx:  4,		/* map index; encodes real idx+1 */
		        :20;		/* reserved */
};
#endif	notdef

/*
 * PTETOPHYS	extract physical address from pte.
 * PHYSTOPTE	turn a physical address into a value OR'able into a pte as the
 *		page-"number".  A NOP for 386; all uses already have zero
 *		low-order bits.
 * PTEPF	extract just the pg_pfnum field in place (and out prot/etc).
 *
 * PTETOPHYS and PTEPF are identical on the 80386; they differed on the 032.
 *
 * PTECLOFF     extracts the memory cluster offset of the page described by
 *              a pte.
 */

#define	PTETOPHYS(pte)	((unsigned)((*(int*)(&(pte))) & PG_PFNUM))
#define	PHYSTOPTE(paddr) ((unsigned)(paddr))
#define	PTEPF(pte)	((unsigned)((*(int*)(&(pte))) & PG_PFNUM))
#define PTECLOFF(pte)   ((unsigned)((*(int*)(&(pte))) & ((~(NBPG-1))&CLOFSET)))

/*
 * Mnemonics for various pte fields.
 */

#define	PG_V		0x00000001		/* valid pte */
#define	PG_W		0x00000002		/* 0=read-only, 1=write */
#define	PG_U		0x00000004		/* 0=system only, 1=user */
#define PG_PWT          0x00000008              /* page write through */
#define PG_PCD          0x00000010              /* page cache disable */
#define	PG_PROT		0x00000006		/* protection field mask */
#define	PG_R		0x00000020		/* reference bit */
#define	PG_M		0x00000040		/* modified bit */
#define	PG_FOD		0x00000080		/* fill-on-demand pte */
#define	PG_FZERO	0x00000100		/* zero-fill-on-demand pte */
#define	PG_MAPX		0x00000F00		/* process local mmap index */
#define	PG_RSVD		0x00000018		/* reserved fields */
#define	PG_PFNUM	0xFFFFF000		/* phys address */

#define	PG_KR		0x00000000		/* valid ==> kernel RW */
#define	PG_KW		0x00000000		/* kernel RW, user NA */
#define	PG_URKW		0x00000004		/* kernel RW, user RO */
#define	PG_UW		0x00000006		/* kernel RW, user RW */

#define	PG_ZFOD		(PG_FZERO|PG_FOD|PG_UW)	/* zero-fill-on-demand pte */

/*
 * Various algorithms assume PTEPF(PG_INVAL) == 0, !PTEMAPPED(PG_INVAL),
 * and (PG_INVAL&PG_FOD) == 0.
 */

#define	PG_INVAL	0			/* totally invalid pte */

/*
 * Pte related macros.
 * L1IDX and L2IDX extract the level-1 and level-2 page-table indicies of 
 * a given address.
 * VADDR() produces the virtual address mapped by given L1 and L2 indicies.
 */

#define	L1IDX(addr)	( ((int)(addr) >> (10+12)) & 0x3FF )
#define	L2IDX(addr)	( ((int)(addr) >> 12) & 0x3FF )
#define	VADDR(l1,l2)	( ((int)(l1) << (10+12)) + ((int)(l2) << 12) )

/*
 * Macros to create and extract u_mmap index in mpte's.
 *
 * PTEMAPPED	true if pte is for mmap'd file (only valid when pg_fod==0).
 * PTETOMAPX	returns map index if pte is mapped.
 * MAPXTOPTE	constructs mask to OR-into pte to store map index.
 */

#define	PG_MAPX_SHIFT	8
#define	PTE_MMAP_MASK	PG_MAPX			/* several refs in older code */

#define	PTEMAPPED(pte)	(((*(int*)(&(pte))) & PG_MAPX) != 0)
#define	PTETOMAPX(pte)	((((*(int*)(&(pte))) & PG_MAPX) >> PG_MAPX_SHIFT) - 1)
#define	MAPXTOPTE(i)	(((int)(i)+1) << PG_MAPX_SHIFT)

/*
 * Pointers into KL2PT, initialized at boot time.
 */

extern	struct pte	*Sysmap;	/* most of kernel */
extern	struct pte	*Usrptmap;	/* user process page-tables/etc */
extern	struct pte	*usrpt;		/* virtual address mapped by Usrptmap */

struct	pte	*vtopte();

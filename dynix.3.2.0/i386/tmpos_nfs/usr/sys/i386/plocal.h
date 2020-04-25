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

/*
 * $Header: plocal.h 2.18 91/02/27 $
 *
 * plocal.h
 *	Processor-Local data-structure declarations, including
 *	private mapping pages and kernel virtual space layout.
 *
 * Includes all data-structures allocated per-processor.
 * Allocated and set up at boot-time.
 *
 * Each processor's plocal is at constant virtual-address, referred
 * to via "l."; thus, l.me == processor logical processor number.
 *
 * Due to shared user+kernel page table, the self-mapped "l." must
 * be saved/restored during context switch.  Thus, a single pte must
 * be copied from out-going level-1 page-table to the incomming one,
 * thus maintaining the *same* copy of this pte in whatever
 * process/page-table runs on a given processor.
 *
 * No per-processor "private" RAM is available on the processor board.
 */

/* $Log:	plocal.h,v $
 */

#include "../machine/gdt.h"

/*
 * The per-processor private data is allocated at boot time.
 * It consists of:
 *
 *		Data-Structure		Size (HW Pages)
 *		==============		===============
 *		Private U-area Pages	UPAGES
 *		Local L1PT		KL1PT_PAGES
 *		Private Mapping Pages	PLMAP_PAGES
 *		Processor Local Data	whatever, rounded to page size
 *
 * Processor local stuff is last since all others are in terms of full
 * pages.
 *
 * A physical address pointer to this is saved in the engine structure of
 * the processor.
 */

/*
 * Processor Local Data.
 * Fields are addressed by "l." -- eg, l.me.  See selfinit() for how this
 * is set up.
 */

struct	plocal	{
	/*
	 * Per-processor Global Descriptor Table (GDT) and Interrupt
	 * Descriptor Table (IDT) to avoid LOCK# contention during
	 * table access (80386 asserts LOCK# when accessing GDT or
	 * IDT, even if table isn't being changed by HW).
	 *
	 * Placed at front of structure to insure 8-byte alignment (the
	 * structure is allocated page aligned).  Unclear if 386 takes
	 * advantage of this, but subsequent processors will.
	 */
	struct	segment_desc	gdt[GDT_SIZE];
	struct	gate_desc	idt[IDT_SIZE];
	/*
	 * Per-processor Task-State Segment (TSS) supplies SS0 and SP0
	 * when enter kernel from user mode.  Allocated per-processor
	 * to avoid problems of "busy" TSS when turn protection on
	 * (a shared system-wide TSS is marked BUSY when loaded into
	 * processor TSS register; the type would have to be cleared
	 * while other processors may be accessing it -- although
	 * this might work, is better to keep it separate).
	 */
	struct	tss_seg	tss;
	/*
	 * Miscellaneous per-processor fields.
	 */
	int		me;		/* Which logical processor am I? */
	struct engine	*eng;		/* "my" engine structure */
	unsigned	panic_pt;	/* saved phys PT addr during panic */
	struct	pte	*priv_pt;	/* phys addr of processor-private PT */
	caddr_t		privstk;	/* top of private stack */
	u_int		plocal_pte;	/* level-1 pte that maps plocal */
	caddr_t		panicsp;	/* panic/pause_self saved sp */
	bool_t		watchptON;	/* boolean for watchpoints ON */
	bool_t		usingfpu;	/* Set if fpu in use */
	int		fpuon;		/* cr0 value to turn ON fpu (287/387) */
	int		fpuoff;		/* value to turn OFF fpu (287/387) */
	int		fpa;		/* 1 == processor has an FPA */
	int		runrun;		/* Set if reschedule requested */
	int		noproc;		/* Set if we are in the switcher */
	int		multprog;	/* # in memory; must sum across l's */
	struct	vmmeter	cnt;		/* various instrumentation */
	daddr_t		rablock;	/* read-ahead block number */
	int		rasize;		/* size of rablock */
	spl_t		splmem;		/* saved spl for memory list */
	int		trap_err_code;	/* trap entries put HW err code here */
	spl_t		vnspl;		/* saved spl for vnode locking */
	bool_t		fpu_pgxbug;	/* processor has FPU Cross Page bug? */
	int		cpu_speed;	/* processor speed */
	int             slic_delay;     /* slic delay count (computed at boot)*/
	int		ptype;		/* processor within the i386 family */
#define PT_I386		0
#define PT_I486		1
#define PT_I586		2
	spl_t		panic_spl;	/* spl at time of panic */
        int             flags;
	int		(*dispatch)();	/* per ptype disptach table */
        int             pad[2];         /* for future expansion */
#ifdef	DEBUG
	bool_t		holdgate;	/* currently holding gate? */
	char		*lastproc;	/* last process dispatched */
#endif	DEBUG
};

/*
 * Flags in plocal "flags" field
 */
#ifdef COBUG
#define PL_C0BUG 1      /* Engine needs to work around C0 i486 FPU bug */
#endif /* COBUG */

/*
 * There must be an integral # pages allocated to the plocal structure,
 * so it can be mapped separate from other data-structures.
 *
 * Parts of the implementation assume PL_PAGES==1.
 * Such places include l.plocal_pte, resume(), vpasspt().
 */

#define	PL_PAGES	btoc(sizeof(struct plocal))

/*
 * Private map structure.
 *
 * Uarea mapping is at top of mapping page to be consistent with Uarea
 * mapping in user processes.  Private map pages are mapped twice in
 * private L1PT; once to map processor local stuff, and once to map
 * private Uarea in virtual address consistent with normal user processes.
 *
 * *** IF CHANGE MAPPING OF U-AREA (machine/vmparam.h), CHANGE THIS TOO ***
 */

#define	PLMAP_RESERVED	(NPTEPG-UPAGES-PL_PAGES)

struct	priv_map {
	struct	pte	pm_plocal[PL_PAGES];	/* processor local data */
	struct	pte	pm_res[PLMAP_RESERVED];	/* reserved, zeroed */
	struct	pte	pm_uarea[UPAGES];	/* current U-area */
};

#define	PLMAP_PAGES	btoc(sizeof(struct priv_map))

/*
 * Processor private pages layout.
 *
 * This just provides a simple way to locate the parts of the structure
 * during initialization.
 *
 * Must be allocated on a 4k boundary since most fields are treated
 * as pages of memory.
 *
 * The L1 page-table entry pointing to pp_pmap[] is context-switch invariant.
 *
 * See machine/vmparam.h for basic address space layout.
 *
 * pp_uarea MUST be first: machine/start.s assumes this.
 */

struct	ppriv_pages {
	char		pp_uarea[UPAGES][NBPG];		/* per-proc Uarea */
	struct	pte	pp_kl1pt[KL1PT_PAGES][NPTEPG];	/* idle/private L1PT */
	struct	pte	pp_pmap[PLMAP_PAGES][NPTEPG];	/* private L2 mapping */
	char		pp_local[PL_PAGES][NBPG];	/* misc vars */
};

#define	SZPPRIV	sizeof(struct ppriv_pages)

#ifdef	KERNEL
extern	struct	plocal	l;
#endif	KERNEL

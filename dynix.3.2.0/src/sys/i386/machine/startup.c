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
static	char	rcsid[] = "$Header: startup.c 2.64 1991/11/06 00:36:56 $";
#endif

/*
 * startup.c
 *	Various startup/initializations.  i386 version.
 *
 * Factored out of machdep.c to keep smaller modules and more related
 * functions together.
 */

/* $Log: startup.c,v $
 *
 *
 *
 *
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../ufs/inode.h"
#include "../ufs/mount.h"
#include "../h/file.h"
#include "../h/clist.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/mbuf.h"
#include "../h/cmn_err.h"
#ifdef QUOTA
#include "../ufs/quota.h"
#endif QUOTA

#include "../balance/engine.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/clkarb.h"
#include "../balance/cfg.h"

#include "../machine/ioconf.h"
#include "../machine/pte.h"
#include "../machine/trap.h"
#include "../machine/hwparam.h"
#include "../machine/plocal.h"
#include "../machine/mmu.h"
#include "../machine/psl.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"
#include "../machine/gate.h"

#include "../machine/scan.h"

/*
 * Define these here, as we fill them out.
 */

int		Usrptsize = 0;		/* # pte's for user page-tables */

int		mono_P_eng;		/* engine for mono_p drivers */

struct	pte	*Sysmap = NULL;		/* start of KL2PT */
struct	pte	*Usrptmap = NULL;	/* KL2PT subset for User PT's */
struct	pte	*Mbmap = NULL;		/* KL2PT subset for mbuf's */
struct	pte	*Bufmap = NULL;		/* KL2PT subset for cache buffers */
struct	pte	*Endmap;		/* End of contiguous level-2 PT */
struct	mbuf	*mbutl;			/* IPC for mbuf init */

struct	pte	*usrpt = NULL;		/* virt-addr mapped by Usrptmap[0] */

struct	pte	*kl1pt_master = NULL;	/* pointer to "master" kl1pt */

int		maxmem = 0;		/* initial free-space */
caddr_t		maxkmem = 0;		/* max kmem virtual address */

#ifdef COBUG
int             c0bug = 1;              /* Patchable use of C0BUG code */
#endif /* COBUG */

static	bool_t	calloc_ok = 1;		/* flag for legal calloc's */
static	caddr_t curmem;			/* memory pointer for calloc() */
char		*calloc();
u_int		memleft();

#define	csalloc(n,type)	(type *) calloc((int)(n)*(sizeof(type)))
#define	valloc(name, type, num) \
		(name) = csalloc(num, type)
#define	valloclim(name, type, num, lim) \
		(name) = csalloc(num, type); (lim) = ((name)+(num))

/*
 * Heuristic limits.
 */

u_long	max_ker_vaddr =	VA_PLOCAL;		/* max kernel virt-address */

/*
 * Tunable flags -- for performance experimentation.
 */

/*
 * Variable(s) to hold kernel virtual address of the SLIC; this changes when we
 * move from powerup environment (PHYS=VIRT) to kernel page tables.  We declare
 * va_slic_lmask to optimize hot code that would otherwise have to compute a
 * constant offset at runtime.
*/

struct  cpuslic *va_slic = (struct cpuslic *)LOAD_CPUSLICADDR;
u_char  *va_slic_lmask = &((struct cpuslic *)LOAD_CPUSLICADDR)->sl_lmask;
int     slic_delay = 0;
u_char  *plocal_slic_delay = (u_char *)&slic_delay;
#ifdef	KXX
int	use_priv_ram = 1;		/* use private RAM */
#endif	/* KXX */

/*
 * Architecture dependent variables
 */
u_char	arch_type;

/*
 * Various externals we need.
 */

extern	int	nbuf;
extern	int	bufpages;
extern	int	bufpct;
extern	int	nswbuf;
extern	int	incr_ptsize;

extern	int	mono_P_slic;
extern	int	resphysmem;

extern	caddr_t	topmem;			/* top of memory */
extern	u_int	totalmem;		/* total memory (topmem-holes) */

extern	struct	ctlr_desc *slic_to_config[];

int     l1pt_cacheable = 0;		/* bitmask (patchable) used to */
					/* turn off caching of L1 PT's */
int     l2pt_cacheable = 0;             /* same for L2 */
int     starttype;                      /* starts out as zero */

/*
 * sysinit()
 *	Do basic system initializations.
 *
 * Includes:
 *	Allocate per-processor local data.
 *	Do heuristics for DS sizing.
 *	Allocate KL2PT (Sysmap et.al.).
 *	Allocate engine structures (per processor).
 *	Fill out (most of) KL2PT, KL1PT.
 *	Init interrupt and trap vectors.
 *
 * Called by 1st processor to start, *very* early after system is alive.
 * Running with segmentation enabled, 1-1 map with physical memory.
 * Paging not yet enabled.
 */

sysinit(startmem, puarea)
	caddr_t	startmem;		/* pointer to first byte of free mem */
	caddr_t	puarea;			/* phys address of U-area base */
{
	register caddr_t addr;
	register struct pte *pte;
	register int i;
	unsigned procid;		/* logical processor */
	struct	ppriv_pages *ppriv;
	struct  cntlrs	*b8k;
	extern	lock_t	time_lck;
	extern	lock_t	panic_lock;
	extern 	int	etext;
	unsigned	cr0;

	/*
	 * Set up architechture dependent changes
	 */

	set_arch_type();

	starttype = lrdslave(va_slic->sl_procid, SL_G_BOARDTYPE);

        if (starttype == SLB_SGS2PROCBOARD) {
		/*
		 * Enable the onchip cache.  Note that we're explicitly
		 * clearing the "Alignment Mask" bit to prevent alignment
	   	 * checking in privledge level 3.
		 */
		inval_cache();
		cr0 = READ_MSW();
		WRITE_MSW((unsigned) (cr0 & ~(CR0_CD|CR0_NW|CR0_AM)));
	}

	/*
 	 * Zap physical U-area; memory not zeroed as we come up,
	 * and need a clean U-area.  Other (per-engine) are zeroed
	 * when allocated, below.
	 */

	bzero(puarea, sizeof(struct user));

	/*
	 * Initialize temporary HW interrupt table.
	 * Need an IDT to take NMI's during probing.
	 * Can't build on "self" local data, since this isn't
	 * yet allocated, and won't be until after configure(),
	 * which determines number of processors.
	 */
	init_temp_IDT(starttype);
	init_temp_GDT();

#ifdef	KXX
	/*
	 * On K20, must replace bytes at zero with inter-segment jump
	 * since processor comes out of reset as a 16-bit 8086, and K20
	 * only re-vectors low 16-bytes.  Real HW will do the intersegment
	 * jump out of cache (treated as local RAM) at high physical addresses.
	 */
	*((char *)0) = 0xea;	/* "jmp sel:offset" in 8086 mode */
	*((char *)1) = 0x44;	/* offset */
	*((char *)2) = 0x00;
	*((char *)3) = 0x00;	/* selector */
	*((char *)4) = 0x00;
#endif	KXX

	/*
	 * Initialize monitor IO (see consio.c)
	 */

	initmonio();

	curmem = startmem;			/* init's calloc() */

        if (starttype == SLB_SGS2PROCBOARD) {
		/*
		 * Init the scan interface and enable NMI.
		 * We need to do this only when booting on an SGS2
	   	 * processors.  Otherwise, the scan interface will
		 * be initialized as part of autoconfig before we
		 * online the first SGS2 processor.
		 */
		scan_init((int)(CD_LOC->c_cons->cd_slic));
		enable_snmi(va_slic->sl_procid);
	}

	/*
	 * Configure the HW and initialize interrupt table (int_bin_table).
	 * Configure() is called with the loader/boot page-tables in use.
	 *
	 * Configure allocates and fills out:
	 *	engine[] array		; one per processor
	 *	Nengine			; # processors
	 *	mono_P_slic		; Slic addr for mono_P drivers
	 *	topmem			; top of physical memory
	 * plus sets up device drivers/etc.
	 */

	init_lock(&panic_lock, G_ENGINE);	/* initialize panic lock */
	va_slic->sl_lmask = 0;			/* insure interrupts OFF */
	configure();

	/*
	 * Initial TLB flush mechanism
	 */

	InitFlushTLB();

	/*
	 * Allow custom system-calls to allocate data-structures.
	 */

	cust_sys_boot();

	/*
	 * Same for bmap caching
	 */

	BmapBoot();

	/*
	 * Allocate fixed-size system data-structures.
	 */

	alloc_tables();

	/*
	 * Allocate processor-local IO mapping pages.
	 */
	
	alloclocalIO();

	/*
	 * Allocate per-processor local data.
	 */

	callocrnd(KL1PT_BYTES);				/* for L1PT's */
	ppriv = (struct ppriv_pages *) calloc((int)Nengine * SZPPRIV);

	/*
	 * Allocate mbuf clusters
	 */

	mclinit();

	/*
	 * Allocate heuristicly-sized system data-structures, including
	 * system level-2 page-table.
	 */

	heuristics(memleft((u_int)curmem));

	/*
	 * Fill out engine structures; these were allocated by
	 * configure() who also filled out the e_slicaddr fields,
	 * and turned on E_FPU387 and/or E_FPA in e_flags if appropriate.
	 *
	 * Figure out my procid from engine structures.
	 *
	 * If there were any mono-P drivers for existing HW, then
	 * they were bound to 'me' (eg, booting processor); thus
	 * set flag to avoid ever taking `me' offline.
	 */

	procid = Nengine;
	for (i = 0; i < Nengine; i++) {
		engine[i].e_local = &ppriv[i];	/* remember private parts */
		engine[i].e_flags |= E_OFFLINE;	/* not up yet */
		/* engine[i].e_slicaddr already filled out */
		if (va_slic->sl_procid == engine[i].e_slicaddr) {
			procid = i;
			if (mono_P_slic >= 0) {
				mono_P_eng = i;
				engine[i].e_flags |= E_DRIVER;
			}
		}
	}
	ASSERT(procid < Nengine, "sysinit: unknown processor");
        /*
         *+ During initialization of the system, the system configuration
         *+ data structures implied that the booting processor was not a
         *+ member of this system.
         */

	init_lock(&engtbl_lck, G_ENGINE);

	/*
	 * "My" engine structure identifies "master" KL1PT.
	 * Init this for KL2PT already known (it got zapped above).
	 */

	kl1pt_master = (struct pte *) &engine[procid].e_local->pp_kl1pt[0][0];

	ASSERT(((int)kl1pt_master & (KL1PT_BYTES-1)) == 0, "kl1pt mis-aligned");
        /*
         *+ The master level-1 page table for the system was not aligned on
         *+ the appropriate boundary.
         */
	ASSERT(((int)Sysmap & (NBPG-1)) == 0, "Sysmap mis-aligned");
        /*
         *+ The kernel data structure Sysmap was not aligned on a page boundary.
         */

	for (i = 0, pte = Sysmap; pte < Endmap; pte += NPTEPG, i++)
		*(int*)(&kl1pt_master[i]) = 
		PHYSTOPTE(pte)|PG_V|PG_KW|PG_R|PG_M |
		l2pt_cacheable;

	/*
	 * Set up Sysmap to map existing physical memory.
	 * Map the text segment as being read only.  Need to make sure the
	 * CR0 register is initialized with the WP bit in order for this
	 * to be effective.  This will prevent the kernel from overwriting
	 * the text segment.  A read of a later edition of the i486 manual
	 * cast doubts on whether this will be allowed.  XXX.jds
	 * Note that physical address space holes are mapped with zeroes in
	 * the page-table.  Could reclaim this memory as free space and zero
	 * the level-1 pte, but (eg) /dev/kmem and meminit() get more
	 * difficult.  Since this isn't all that big, ignore for now (relevant
	 * cmap array space is much larger).
	 *
	 * This assumes kernel is linked above CD_LOC.
	 */
	for (i = btop(CD_LOC), addr = (caddr_t)CD_LOC; addr < (caddr_t)&etext;
							    addr += NBPG, i++) {
		if (page_exists(i)) {
			*(int *)(&Sysmap[i]) = PHYSTOPTE(addr)
		     				| PG_V|PG_KR|PG_R|PG_M;
	    	}
	}

	for (; addr < topmem+resphysmem; addr += NBPG, i++) {
		if (page_exists(i)) {
	    		*(int *)(&Sysmap[i]) = PHYSTOPTE(addr)
				   		| PG_V|PG_KW|PG_R|PG_M;
		}
 	}
	/*
	 * Initialize IO controller mapping.
	 */

	for (b8k = b8k_cntlrs; b8k->conf_b8k != NULL; b8k++)
		(*b8k->b8k_map)(kl1pt_master);

	/*
	 * Initialize time_lck. This must be done before the 1st
	 * tod clock interrupt as this triggers a softclock interrupt
	 * which uses this lock.
	 */

	init_lock(&time_lck, G_TIME);

	/*
	 * Init "eng_wait" semaphore, used to coordinate bringing
	 * processors on-line and taking processors off-line.
	 * Initialize here since selfinit does a V. Need to reinitialize
	 * after selfinit().
	 */

	init_sema(&eng_wait, 0, 0, G_ENGINE);

	/*
	 * System is mapped enough to do self-init's.
	 * Need to do this now, since allockds() needs to run mapped.
	 */

	selfinit(procid);

	/*
	 * Enable Ecc error reporting in memory controllers.
	 *
	 * Note: errors will merely be latched.  No interrupts
	 * are generated other than NMI for uncorrectable errors.
	 */

	memenable();

	/*
	 * Reinitialize since selfinit does a V sema.
	 */

	init_sema(&eng_wait, 0, 0, G_ENGINE);

	/*
	 * Good {morning,afternoon,evening,night}.
	 */

	CPRINTF(version);

#define MEG	(1024*1024)
	CPRINTF("real mem  = %d.%d%d megabytes.\n", totalmem/MEG,
		((totalmem%MEG)*10)/MEG,
		((totalmem%(MEG/10))*100)/MEG);

	/*
	 * Initialize callouts
	 */

	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	/*
	 * Initialize memory allocator and user page table maps.
	 */

	maxmem = meminit(curmem);
	i = ctob(maxmem);
	CPRINTF("avail mem = %d.%d%d megabytes.\n", i/MEG,
		((i%MEG)*10)/MEG,
		((i%(MEG/10))*100)/MEG);

	i = bufpages * CLBYTES;
	CPRINTF("using %d buffers containing %d.%d%d megabytes of memory.\n",
		nbuf,
		i/MEG,
		((i%MEG)*10)/MEG,
		((i%(MEG/10))*100)/MEG);

	uptinit((long)Usrptsize, UPTMAPMULT*nproc);
	rminit(mbmap, (long)((nmbclusters - 1) * CLSIZE), (long)CLSIZE,
	    "mbclusters", nmbclusters/4);

	/*
	 * Hand-craft page-table for process[0] -- swapper.
	 * Need to set this up before call main() from start.s, since
	 * currently running on boot-processor's private resources.
	 *
	 * Use of vgetpt() inherits current l. mapping.
	 *
	 * NOTE: currently running on proc[0]'s Uarea stack, but at
	 * physical address.  "u." references are to processor private Uarea.
	 * All proc[] fields were zeroed when struct proc's were allocated.
	 */

	proc[0].p_szpt = SZPT(&proc[0]);		/* includes Uarea map */
	proc[0].p_ptb1 = (unsigned) l.priv_pt;		/* for vmaccess() */
	u.u_procp = &proc[0];				/* ditto */
	(void) vgetpt(&proc[0], vmemall);		/* alloc page-table */
	*(int *) UAREAPTES(&proc[0]) = PHYSTOPTE(puarea) | PG_V|PG_KW|PG_R|PG_M;
	WRITE_PTROOT(proc[0].p_ptb1);

#ifdef	DEBUG
	/*
	 * Set up process 0's stack with known pattern -- allows sense of
	 * stack overflow more easily.  Needs vgetu() to copy entire Uarea
	 * if this is to propogate.
	 */
	for (i=0; (caddr_t)&u.u_stack[i] <= (caddr_t)&u+ctob(UPAGES)-512; i++)
		u.u_stack[i] = (i & 1) ? 0xdeadbeef : 0xfeedface;
#endif	DEBUG
}


/*
 * set_arch_type()
 *
 * Set global "arch_type" from CFG data.
 */

set_arch_type()
{
	arch_type = CD_LOC->c_sys.sd_type;

	switch (arch_type) {

	case SYSTYP_S1:
		/*
		 * EISA hardware platforms
		 */
	default:
		cmn_err(CE_PANIC,
			"set_arch_type: unsupported type %d\n", arch_type);
		/*
		 *+ The config data tables contain a value for
		 *+ the system type that is unsupported.
		 */
		break;

	case SYSTYP_S27:
	case SYSTYP_S81:
	case SYSTYP_S16:
		/*
		 * SYMMETRY hardware platforms
		 */
		break;
	}
}


/*
 * alloc_tables()
 *	Allocate (most) fixed-size kernel data-structures.
 *
 * This used to be called startup() in 4.2bsd, and was called by main().
 */

static
alloc_tables()
{
	valloc(mbmap, struct map, nmbclusters/4);
	valloc(cfree, struct cblock, nclist);
	valloclim(inode, struct inode, ninode, inodeNINODE);
	valloclim(file, struct file, nfile, fileNFILE);
	valloclim(proc, struct proc, nproc, procNPROC);
	valloclim(mounttab, struct mount, nmount, mountNMOUNT);
	valloc(callout, struct callout, ncallout);
	valloc(mfile, struct mfile, nmfile);
	valloc(ucred_free, struct ucred, nucred);
	valloc(swapmap, struct map, nswapmap = nproc * 2);
	valloc(argmap, struct map, ARGMAPSIZE);
	valloc(uptmap, struct map, UPTMAPMULT*nproc);
	ofile_boot();
#ifdef QUOTA
	if (ndquot) {
		valloclim(dquot, struct dquot, ndquot, dquotNDQUOT);
		valloc(dqhead, struct dqhead, ndqhash);
	}
#endif
}

/*
 * heuristics()
 *	Allocate heuristicly sized system data structures.
 */

static
heuristics(freesize)
	u_int	freesize;
{
	register char	*paddr;
	register int	mapidx;
	register int	j;
	register unsigned i;
	register int	base;
	register int	residual;
	extern	short	Mbmapsize;
	extern	int	avg_size_process;		/* in bytes */
	extern	int	buf_hash_mult, buf_hash_div;

	/*
	 * Figure size of Usrptmap[] (user page-tables part of KL2PT).
	 * Base this on nproc, and layout of a process (U-area, page-table,
	 * sizing page-table based on "average" size process (see conf/param.c).
	 * Each pte in Usrptmap[] maps *alot* of space, and there is *alot*
	 * of kernel virtual space; thus no big deal if overallocate a bit.
	 *
	 * Tuning of "nproc" might be more appropriate; suspect the
	 * heuristic in param.c is too conservative.
	 *
	 * The idea is to not prematurely swap due to lack of Usrptmap
	 * space, but not overallocate Usrptmap either.
	 */

	if (Usrptsize == 0) {
		Usrptsize = incr_ptsize + nproc *
				( UPAGES		/* U-area */
				+ UL1PT_PAGES		/* UL1PT */
				+ SZL2PT(btoc(avg_size_process),1)
				+ CLSIZE		/* Slop */
				);
	}
	Usrptsize = clrnd(Usrptsize);

	/*
	 * Determine how many buffers to allocate.
	 * Use bufpct % of memory, with min of 16.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 *
	 * Since max physical memory is about 256Meg, 10% of this is
	 * 25Meg.  With 4K page-size, and one buf-page per virtual
	 * buffer, this is 50Meg address space.  Since user stuff
	 * starts at 1Gig, very easy to overflow; must be careful.
	 */

	if (bufpages == 0) {
		/*
		 *  We are careful to avoid overflow for large memory
		 *  systems....
		 */
		bufpages = (freesize / (100*CLBYTES)) * bufpct
			 + (freesize % (100*CLBYTES)) * bufpct / (100*CLBYTES);
	}
	if (nbuf == 0) {
		j = ((int)(max_ker_vaddr - (ulong)(topmem + resphysmem)) -
					(int)ptob(Usrptsize + Mbmapsize)) /
					          MAXBSIZE;
		nbuf = MIN(bufpages, j);
	}
	if (nbuf < 16)
		nbuf = 16;

	if (nswbuf == 0) {
		nswbuf = nbuf / 2;
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	if (bufhsz == 0)
		bufhsz = imax(nbuf * buf_hash_mult / buf_hash_div, MIN_BUFHSZ);

	valloclim(bufhash, struct bufhd, bufhsz, bufhashBUFHSZ);
	valloc(buf, struct buf, nbuf);
	valloc(swbuf, struct buf, nswbuf);

	/*
	 * Determine # device-vnodes to allocate.
	 * Heuristic is one per possible partition on each useful block device
	 * (in standard cfg 4 are not useful), assuming 4 useful partitions
	 * is the "norm", and 6 drives per blkdevsw[] entry.
	 */

	if (ndevnode == 0)
		ndevnode = (nblkdev - 4) * 4 * 6;
	valloc(devnode, struct vnode, ndevnode);

	/*
	 * Allocate KL2PT.  Form:
	 *
	 *	Sysmap
	 *	Usrptmap
	 *	Mbmap
	 *	Bufmap
	 *
	 * Sysmap[] is sized to map all of physical memory.
	 *
	 * Create entire L2 page-table in one allocation since code assumes
	 * contiguous KL2PT.  Could make filling out of kl1pt smarter but
	 * holes skipped here should be rare.
	 *
	 * Note: Sysmap and Usrptmap sizes must be multiples of CLSIZE
	 * to align Mbuf's.
	 */

	callocrnd(NBPG);

	i = clrnd((int) btop(topmem + resphysmem))	/* Sysmap */
	  + Usrptsize					/* Usrptmap */
	  + Mbmapsize					/* Mbmap */
	  + nbuf * btop(MAXBSIZE);			/* Bufmap */

	ASSERT((i * sizeof(struct pte)) < (topmem - curmem),
		"heuristics: insufficent memory for level 2 page tables");
	/*
	 *+ Insufficent memory is available to allocate the user level
	 *+ 2 page tables. This indicates a resource configuration
	 *+ problem.
	 *+ Corrective action:  decrease the number of user process slots
	 *+ or add more memory.
	 */

	Sysmap = csalloc(i, struct pte);
	Usrptmap = Sysmap + clrnd((int) btop(topmem + resphysmem));
	Mbmap = Usrptmap + Usrptsize;
	Bufmap = Mbmap + Mbmapsize;
	Endmap = &Sysmap[i];

	usrpt = (struct pte *) ptob(Usrptmap - Sysmap);
	mbutl = (struct mbuf *) ptob(Mbmap - Sysmap);
	buffers = ptob(Bufmap - Sysmap);

	maxkmem = ptob(Endmap - Sysmap);
	ASSERT((u_long)maxkmem <= max_ker_vaddr, "heuristics: maxkmem");
        /*
         *+ When sizing memory and allocating kernel data structures,
         *+ the kernel's notion of the highest kernel address overflowed
         *+ into virtual space used to map processor-local information.
         */

	/*
	 * Fill out buf-headers by distributing bufpages among the headers.
	 * binit() fills in b_addr.
	 */

	callocrnd(CLBYTES);

	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	mapidx = 0;

	for (i = 0; i < residual; i++) {
		valloc(paddr, char, ptob((base + 1) * CLSIZE));
		for (j = 0; j < (base + 1) * CLSIZE; j++) {
			*(int *)(&Bufmap[mapidx+j]) = PHYSTOPTE(paddr)
						    | PG_V|PG_R|PG_M | PG_KW;
			paddr += NBPG;
		}
		mapidx += MAXBSIZE / NBPG;
	}
	for (i = residual; i < nbuf; i++) {
		valloc(paddr, char, ptob(base * CLSIZE));
		for (j = 0; j < base * CLSIZE; j++) {
			*(int *)(&Bufmap[mapidx+j]) = PHYSTOPTE(paddr)
						    | PG_V|PG_R|PG_M | PG_KW;
			paddr += NBPG;
		}
		mapidx += MAXBSIZE / NBPG;
	}

	/*
	 * Allocate space for core map.
	 * Allocate a cmap structure for each page of memory < topmem (including
	 * holes).  meminit() handles reclaiming useless cmap[] pages.
	 * Cluster align cmap[] to make reclaim of holes more likely.
	 */

	callocrnd(CLBYTES);
	valloclim(cmap, struct cmap, (int) topmem / CLBYTES, ecmap);

	ASSERT(curmem < topmem - 8*UPAGES, "heuristics: no memory");
        /*
         *+ After allocating kernel data structures, the kernel found that
         *+ no memory was available for user processes.
         */

	/*
	 * There should be NO allocations after this!
	 */

	callocrnd(CLBYTES);
	calloc_ok = 0;				/* insure no more calloc()'s */
}

/*
 * selfinit()
 *	Do self-init's.  Done by each processor as it comes alive.
 *
 * If system just booting, sysinit() has already been called to
 * allocate kernel data-structures and init common parts kernel
 * level-2 page-tables.
 *
 * Includes:
 *	Init self KL1PT.
 *	Init per-processor descriptor tables (GDT, IDT).
 *	Init private parts of processor page-tables.
 *	Map U-area.
 *	Turn ON page-table mapping.
 *	(K20 only) Copy stuff to private RAM and start using this.
 *	Init interrupts.
 *	Init any other local processor resources.
 *
 * Caller arranged that we're already running on a U-area stack.
 */

selfinit(procid)
	unsigned procid;			/* logical processor # */
{
	register struct	ppriv_pages *ppriv;	/* my private parts */
	register struct	pte	*kl1pt;		/* my KL1PT */
	register int	i;
	struct	priv_map *pm;			/* my private mapping pages */
	struct	engine	*eng;			/* my engine structure */
	extern	bool_t	light_show;
	int	ver386;
	unsigned	cr0;

	eng = &engine[procid];
	ppriv = eng->e_local;

	/*
	 * Copy private KL1PT from "master" copy -- that of 1st
	 * processor booted.  This is redundant for 1st booted processor.
	 */

	kl1pt = &ppriv->pp_kl1pt[0][0];
	if (kl1pt != kl1pt_master)
		bcopy((caddr_t)kl1pt_master, (caddr_t)kl1pt, KL1PT_BYTES);

	ASSERT_DEBUG(((int)kl1pt & (KL1PT_BYTES-1)) == 0, "kl1pt mis-aligned");

	/*
	 * Make private PT self-referencing.
	 */

  	*(int*) &kl1pt[L1IDX(VA_PT)] = 
				PHYSTOPTE(kl1pt) | PG_V|PG_R|PG_M|PG_KW 
				| l2pt_cacheable;

	/*
	 * Make private-mapping pages map private stuff and Uarea, and
	 * map these in the level-1 page-table.
	 */

	pm = (struct priv_map *) &ppriv->pp_pmap[0][0];
	
  	*(int*) &kl1pt[L1IDX(VA_PLOCAL)] = 
					PHYSTOPTE(pm) | PG_V|PG_R|PG_M|PG_KW
					| l2pt_cacheable;

	for (i = 0; i < PL_PAGES; i++)
		*(int *) &(pm->pm_plocal[i]) =
			PHYSTOPTE(&ppriv->pp_local[i][0]) |PG_V|PG_R|PG_M|PG_KW;

  	*(int*) &kl1pt[L1IDX(VA_UAREA)] = 
					PHYSTOPTE(pm) | PG_V|PG_R|PG_M|PG_KW
					| l2pt_cacheable;

	for (i = 0; i < UPAGES; i++)
		*(int *) &(pm->pm_uarea[i]) =
			PHYSTOPTE(&ppriv->pp_uarea[i][0]) |PG_V|PG_R|PG_M|PG_KW;

	/*
	 * Map processor local-IO stuff.
	 */

	maplocalIO(kl1pt);

#ifdef	KXX
	/*
	 * Copy stuff into private RAM.  Must be done before turning
	 * ON paging, since private RAM not mapped by standard PT, but
	 * is within segment bounds.
	 *
	 * WARNING: The wrslave() routine *MUST* reside in private ram
	 * in order to properly initialize the cache. There is a race
	 * condition in the hardware such that one cache block is
	 * installed incorrectly.  This causes serious failures if the
	 * code now resident in the cache is used.
	 */
	if (use_priv_ram)
		setup_priv_ram(kl1pt);
#endif	KXX

	/*
	 * Initialize descriptor tables, and turn ON mapping!
	 */

	bzero((caddr_t) &ppriv->pp_local[0][0], sizeof(struct plocal));

	init_desc_tables((struct plocal *) &ppriv->pp_local[0][0]);
	WRITE_PTROOT((u_int)kl1pt | l1pt_cacheable);/* set up page-table root */
	cr0 = READ_MSW();
	if (eng->e_flags & E_FPU387)		/* if 387... */
		WRITE_MSW((unsigned) (cr0 |CR0_PG|CR0_PE|CR0_EM|CR0_MP));
	else
		WRITE_MSW((unsigned) (cr0 |CR0_PG|CR0_PE|CR0_EM));

	/*
	 * Mapping may move address of the SLIC so rewrite
	 * variables to reflect this change.
	 */
	va_slic = (struct cpuslic *) VA_SLIC;
	va_slic_lmask = &((struct cpuslic *) VA_SLIC)->sl_lmask;
	plocal_slic_delay = (u_char *)&l.slic_delay;

	/*
	 * Fill out relevant fields in "l" structure.
	 * Some of these (eg, eng, priv_pt) are derivable, but placed
	 * here to allow fast access during system execution.
	 */

	l.me = procid;				/* "me" */
	l.eng = eng;				/* "me", too */
	l.priv_pt = kl1pt;			/* for use_private() */
	l.privstk = ppriv->pp_uarea[UPAGES];	/* top of private stack */
	l.plocal_pte = (int) ppriv->pp_pmap[0] | PG_V|PG_R|PG_M|PG_KW;
	l.noproc = 1;				/* no process running yet! */
	l.fpuon = (CR0_PG|CR0_PE);		/* how to turn FPU on */
	if (eng->e_flags & E_FPU387) {		/* if 387... */
		l.fpuon |= CR0_ET|CR0_MP;	/*	... set for 387 */
		/* i486--turn on numeric error too */
		if (eng->e_flags & E_SGS2)
			l.fpuon |=  CR0_NE;

		l.fpuoff = l.fpuon | CR0_EM;	/* off ==> emulate math */
		init_fpu();			/* 387 needs fninit */
	} else {
		l.fpuon |= CR0_EM;		/*	... set for 387 */
		l.fpuoff = l.fpuon;		/* off ==> emulate math */
	}

	/*
	 * If processor is early rev, set l.fpu_pgxbug to imply checking
	 * for this chip-bug should be done.  Do this always on K20, and
	 * read config proms to determine if D step 386 and set if not.
	 * This applies only to 386.
	 */
#ifdef	KXX
	l.fpu_pgxbug = 1;			/* always on K20 */
#else
        if (!(eng->e_flags & E_SGS2)) {
		ver386 = SL_P2_386_VER(eng->e_slicaddr);
		l.fpu_pgxbug = (ver386 != SL_P2_386_D);
	}
#endif	KXX

#ifdef COBUG
	/*
	 * C0-step and before i486 have a bug in their FPU, flag that
	 * we need to work around it. XXX we need to read config PROM,
	 * but we don't have the values yet.
	 */
	if ((eng->e_flags & E_SGS2) && c0bug) {
		int ver486;

		ver486 = SL_P2_486_VER(eng->e_slicaddr);
		if (ver486 <= SL_P2_486_C1) {
			l.flags |= PL_C0BUG;
		}
	}
#endif /* COBUG */
	/*
	 * Do other processor-local inits.
	 */

	localinit();

	/*
	 * Fill out the engine structure.
	 * Need to do after localinit(), to avoid switcher posting
	 * dispatch SW interrupt to us until we've enabled (properly)
	 * SLIC interrupts.
	 */

	eng->e_state = E_GLOBAL;			/* not bound */
	eng->e_pri = eng->e_npri = PIDLE;		/* idle */
	eng->e_head = eng->e_tail = (struct proc *)eng;	/* no affinity */
	eng->e_count = 0;				/* no affinity */
	eng->e_flags &= ~E_OFFLINE;			/* on-line, now! */

	if (eng->e_flags & E_SGS2)
		/*
		 * Enable NMIs
		 */
		enable_snmi(eng->e_slicaddr);

	/*
	 * We're *on*!  Need to goose "eng_wait" sema to complete the init.
	 * This allows another on-line or off-line to start.
	 */

	v_sema(&eng_wait);

	/*
	 * Say hello.
	 *
	 * Processor up -- turn processor LED on.
	 */

	if (light_show) {
		DISABLE();
		if (fp_lights)
			FP_LIGHTON(l.me);
#ifdef	KXX
		if (fp_lights <= 0)
			(void) rdslave(l.eng->e_slicaddr, SL_P_LIGHTON);
#else
		*(int *) PHYS_LED = 1;
#endif	KXX
		ENABLE();
	}
}

/*
 * alloclocalIO()
 *	Allocate processor-local IO mapping pages.
 *
 * These are shared by all processors.
 *
 * When/if need to map more such things, should make this table-driven.
 */

static	struct	pte	*slic_map;		/* page to map the SLIC */

#ifndef	KXX
static	struct	pte	*led_map;		/* page to map the LED */

#if	defined(DEBUG) || defined(MFG)
static	struct	pte	*sync_point_map;	/* page to map sync-points */
#endif	DEBUG||MFG
#endif	KXX

static
alloclocalIO()
{
	/*
	 * Allocate mapping for FPA.
	 * Not mapped in maplocalIO(); rather let page fault turn it on
	 * per process.
	 */

	alloc_fpa();

	/*
	 * Allocate SLIC map.
	 */

	callocrnd(NBPG);
	slic_map = csalloc(NPTEPG, struct pte);
	*(int *) &slic_map[L2IDX(PHYS_SLIC)] =
			PHYSTOPTE(PHYS_SLIC) | PG_V|PG_R|PG_M|PG_KW|
					       PG_PCD | PG_PWT;

#ifdef	KXX
	/*
	 * Map private RAM physical == virtual (mostly for /dev/kmem).
	 */
	*(int *) &slic_map[L2IDX(PHYS_STATICRAM+0*NBPG)] =
			PHYSTOPTE(PHYS_STATICRAM+0*NBPG) | PG_V|PG_R|PG_M|PG_KW;
	*(int *) &slic_map[L2IDX(PHYS_STATICRAM+1*NBPG)] =
			PHYSTOPTE(PHYS_STATICRAM+1*NBPG) | PG_V|PG_R|PG_M|PG_KW;
#endif	KXX

#ifndef	KXX
	/*
	 * Allocate map to talk to processor LED.
	 */

	callocrnd(NBPG);
	led_map = csalloc(NPTEPG, struct pte);
	*(int *) &led_map[L2IDX(PHYS_LED)] =
			PHYSTOPTE(PHYS_LED) | PG_V|PG_R|PG_M|PG_KW|
					      PG_PCD | PG_PWT;

	/*
	 * Also map processor Elapsed Time Counter (ETC).
	 * Goes in same level-2 page as LED on Symmetry.
	 */

	ASSERT_DEBUG(*(int*)&led_map[L2IDX(PHYS_ETC)]==0, "alloclocalIO: ETC");
	*(int *) &led_map[L2IDX(PHYS_ETC)] =
			PHYSTOPTE(PHYS_ETC) | PG_V|PG_R|PG_M|PG_KR |
					      PG_PCD | PG_PWT;

#if	defined(DEBUG) || defined(MFG)
	/*
	 * Allocate map to talk to processor "synch-points".
	 * Set up 4Meg of such address space, starting at virtual
	 * PHYS_SYNC_POINT; addresses physical PHYS_SYNC_POINT for 4Meg.
	 * Assumes synch-points start on 4Meg address space boundary.
	 */
	callocrnd(NBPG);
	sync_point_map = csalloc(NPTEPG, struct pte);
	{	int	i;
		for (i = 0; i < NPTEPG; i++) {
			*(int *) &sync_point_map[i] =
					PHYSTOPTE(PHYS_SYNC_POINT + ptob(i))
					| PG_V|PG_R|PG_M|PG_KW |
					  PG_PCD | PG_PWT;
		}
	}
#endif	DEBUG||MFG
#endif	KXX
}

/*
 * maplocalIO()
 *	Make sure processor-local resources are mapped by given page-table.
 */

static
maplocalIO(kl1pt)
	register struct pte *kl1pt;
{
	/*
	 * Map the SLIC.
	 *
	 * Note that this consumes addresses 24Meg-28meg on the K20;
	 * if the system has more than 24Meg of memory, this 4Meg is not
	 * 1-1 mapped.  machine/hwparam.h limits K20 usable memory to
	 * 24Meg to deal with this.  If K20 were to become production,
	 * could just take over the top 64K if there was 28Meg.
	 */

	if (*(int *) &kl1pt[L1IDX(PHYS_SLIC)]
			!= (PHYSTOPTE(slic_map) | PG_V|PG_R|PG_M|PG_KW |
			    l2pt_cacheable)) {
		ASSERT(*(int*)&kl1pt[L1IDX(PHYS_SLIC)] == 0,"maplocalIO: SLIC");
                /*
                 *+ On initialization, the SLIC was mapped 
		 *+ to an invalid address.
                 */
		*(int *) &kl1pt[L1IDX(PHYS_SLIC)] = 
				PHYSTOPTE(slic_map) | PG_V|PG_R|PG_M|PG_KW |
				l2pt_cacheable;
	}

#ifndef	KXX
	/*
	 * Map the LED (1st time only).
	 */

	if (*(int *) &kl1pt[L1IDX(PHYS_LED)]
			!= (PHYSTOPTE(led_map) | PG_V|PG_R|PG_M|PG_KW |
			    l2pt_cacheable)) {
		ASSERT(*(int*)&kl1pt[L1IDX(PHYS_LED)] == 0, "maplocalIO: LED");
                /*
                 *+ On initialization, the LED was mapped to an 
	 	 *+ invalid address.
                 */

		*(int *) &kl1pt[L1IDX(PHYS_LED)] = 
				PHYSTOPTE(led_map) | PG_V|PG_R|PG_M|PG_KW |
				l2pt_cacheable;
	}

#if	defined(DEBUG) || defined(MFG)
	/*
	 * Map SYNC_POINT (1st time only).
	 */
	if (*(int *) &kl1pt[L1IDX(PHYS_SYNC_POINT)]
			!= (PHYSTOPTE(sync_point_map) | PG_V|PG_R|PG_M|PG_KW |
			    l2pt_cacheable)) {
		ASSERT(*(int*)&kl1pt[L1IDX(PHYS_SYNC_POINT)] == 0, "maplocalIO: SYNC_POINT");
		*(int *) &kl1pt[L1IDX(PHYS_SYNC_POINT)] = 
				PHYSTOPTE(sync_point_map) | PG_V|PG_R|PG_M|PG_KW |
				l2pt_cacheable;
	}
#endif	DEBUG||MFG
#endif	KXX
}

#ifdef	KXX
/*
 * setup_priv_ram()
 *	Set up and use the private RAM.  Just places low-order 8K of
 *	system memory in the fast RAM.
 *
 * We are NOT running on page-tables yet.
 */

/*
 * Assume kernel linked to be based at 16k -- new CFG table starts at
 * 8k; starting kernel at 16k leaves room for expansion.  This can probably
 * be done better to determine actual load base from symbols in the kernel.
 */

#define	LOAD_BASE	(16*1024)	/* under new FW, load at 16k */

static
setup_priv_ram(kl1pt)
	register struct	pte *kl1pt;
{
	register struct pte *pte;

	/*
	 * Copy low-order 8K of physical memory into private RAM.
	 */

	bcopy((caddr_t)LOAD_BASE, (caddr_t)PHYS_STATICRAM, 8*1024);

	/*
	 * Map it with page-table.  Note that all page-tables
	 * will have the same stuff here, since PHYS_STATICRAM
	 * is constant across boards.
	 *
	 * This code "knows" NBPG == 4k, and that LOAD_BASE fits in 1st
	 * level-2 page.
	 */

	pte = (struct pte *) PTETOPHYS(kl1pt[0]);
	*(int *) &pte[LOAD_BASE/NBPG+0] = PHYSTOPTE(PHYS_STATICRAM + 0*NBPG)
					| PG_V|PG_R|PG_M|PG_KW;
	*(int *) &pte[LOAD_BASE/NBPG+1] = PHYSTOPTE(PHYS_STATICRAM + 1*NBPG)
					| PG_V|PG_R|PG_M|PG_KW;
}
#endif	KXX

/*
 * localinit()
 *	Init local processor resources.
 *
 * This involves:
 *	turning on the cache,
 *	setting up SLIC interrupt control,
 *	set up and start SLIC local clock.
 *
 * Assumes configure() set up interrupt vector for local clock handler,
 * since we turn on the local clock here.
 */

static
localinit()
{
	extern	 int	lcpuspeed;
	extern	 int	i486_lcpuspeed;
	register struct cpuslic *sl = va_slic;
#ifdef	KXX
	register u_int	diag_flag;
	register u_int	cache_flags;
	static	char	cache_off[] =
		"Processor %d (slic %d) running with cache set %d disabled.\n";

	/*
	 * Turn on NMI's first and zap cache.
	 * Then, enable cache.
	 *
	 * Enable cache sets and write-buffer only if diagnostics don't
	 * say they're bad.
	 */

	diag_flag = l.eng->e_diag_flag;
	cache_flags = SLB_ENB0|SLB_INV0|SLB_ENB1|SLB_INV1|SLB_E_WR_BUF;
	if (diag_flag & CFG_CACHE0) {			/* set 0 is bad */
		cache_flags &= ~SLB_ENB0;
		CPRINTF(cache_off, l.me, l.eng->e_slicaddr, 0);
	}
	if (diag_flag & CFG_CACHE1) {			/* set 1 is bad */
		cache_flags &= ~SLB_ENB1;
		CPRINTF(cache_off, l.me, l.eng->e_slicaddr, 1);
	}
	if (diag_flag & CFG_WBUF) {			/* write-buf is bad */
		cache_flags &= ~SLB_E_WR_BUF;
		CPRINTF("Processor %d (slic %d) running with write-buffer disabled.\n",
				l.me, l.eng->e_slicaddr);
	}

	/*
	 * Wrslave routine must execute out of privare on-board ram in order
	 * to initialize the cache properly.
	 */

	l.cntrlreg = (u_char)(SLB_E_NMI|cache_flags);
	wrslave(sl->sl_procid, SL_P_CONTROL, SLB_E_NMI);
	wrslave(sl->sl_procid, SL_P_CONTROL, (u_char)(SLB_E_NMI|cache_flags));

#else	Real HW
	/*
	 * Nothing required.  start_engine() brought processor up with
	 * PROC_CTL register in apprioriate state (including NMI's enabled).
	 */
#endif	KXX

	/*
	 * If processor has an FPA, initialize it.
	 */

	if (l.eng->e_flags & E_FPA) {
		l.fpa = 1;
		init_fpa();
		online_fpa();
	}

	/*
	 * compute l.cpu_speed for delay loops. The vaule should correspond
	 * to the "mips" for the processor at its running rate.
	 * e_clock_rate: is a actual board running rate
	 * lcpuspeed: is the number of mips if e_cpu_speed = 100
	 *
	 */
	if (l.eng->e_flags & E_SGS2) {
		l.ptype = PT_I486;
		/*
		 * Use i486 ipc rather than i386.
		 */
		l.cpu_speed = (i486_lcpuspeed * l.eng->e_cpu_speed) /100;
	} else {
	 	l.cpu_speed = (lcpuspeed * l.eng->e_cpu_speed) / 100;
		l.ptype = PT_I386;
	}
	/*
	 * Fill in l.slic_delay field.  For models B, C, and D the overhead
	 * of the "countdown" loop for the slic lmask delay more than cover
	 * the needed time.  For faster models, the count may need to be
	 * non-zero.
	 */
	l.slic_delay = 0;
	/*
	 * Set up SLIC interrupt control and start local clock.
	 */

	(void) splhi();				/* all intrs masked */
	ENABLE();				/* but ON at processor */

#ifdef	DEBUG
	if ((sl->sl_ictl & (SL_HARDINT|SL_SOFTINT)) != 0) {
		printf("localinit: pending interrupts 0x%x\n", 
				sl->sl_ictl & (SL_HARDINT|SL_SOFTINT));
		panic("localinit");
	}
#endif	DEBUG
	ASSERT((sl->sl_ictl & (SL_HARDINT|SL_SOFTINT)) == 0, "localinit: pending ints");
       	/*
         *+ On initialization, the processor found 
	 *+ interrupts pending at the SLIC.
	 */
	sl->sl_ictl = 0x00;			/* not using `m' bit */

	sl->sl_procgrp = TMPOS_GROUP;		/* set group ID */
	setgm(sl->sl_procid, SL_GM_ALLON);	/* set self group-mask all ON */
#ifdef	CHECKSLIC
	ASSERT(sl->sl_gmask == SL_GM_ALLON, "localinit: GM not set");
#endif	CHECKSLIC

	startrtclock();				/* start local timer */

	(void) spl0();				/* intr's unmasked -- go! */
}

/*
 * init_temp_IDT()
 *	Set up temporary IDT to allow taking NMI's during probing.
 */

static	struct	gate_desc temp_IDT[T_NMI+1];	/* temporary IDT */
static	struct	desctab	desctab;		/* to init IDT, GDT */
static	u_short	selector;			/* temporary selector */
static  struct  segment_desc temp_GDT[GDT_KDS+1];/* temporary GDT */

static
init_temp_IDT(type)
	int type;
{
	register struct gate_desc *gd = &temp_IDT[T_NMI];
	extern	t_nmi();

	/*
	 * Get stand-alone CS selector, so can build interrupt gate.
	 */

	asm("movw %cs, %ax");
	asm("movw %ax, _selector");

	/*
	 * Construct interrupt gate for NMI.
	 */

	BUILD_GATE_DESC(gd, selector, t_nmi, INTR_GATE_TYPE, DPL_KERNEL, 0);

	/*
	 * Set up IDT by building a "table-descriptor" and loading
	 * IDT register with it.
	 */

	BUILD_TABLE_DESC(&desctab, temp_IDT, T_NMI+1);
	asm("lidt _desctab");

	/*
	 * Enable NMI's on the processor.
	 */

#ifdef	KXX
	lwrslave(va_slic->sl_procid, SL_P_CONTROL, SLB_E_NMI);
#else
	if (type != SLB_SGS2PROCBOARD)
		lwrslave(va_slic->sl_procid, PROC_CTL,
		PROC_CTL_NO_SSTEP | PROC_CTL_NO_HOLD | PROC_CTL_NO_RESET);
#endif	KXX
}

static
init_temp_GDT()
{
	/*
 	 * Construct interrupt gate for NMI.
  	 */

	BUILD_SEG_DESC(&temp_GDT[GDT_KCS], 0, SD_MAX_SEG,
					CODE_SEG_TYPE, DPL_KERNEL);
	BUILD_SEG_DESC(&temp_GDT[GDT_KDS], 0, SD_MAX_SEG,
					DATA_SEG_TYPE, DPL_KERNEL);
	BUILD_TABLE_DESC(&desctab, temp_GDT, GDT_KDS+1);
	asm("lgdt _desctab");
}
/*
 * init_desc_tables()
 *	Set up processor-local copy of GDT and IDT, and start using them.
 */

int	t_diverr(), t_dbg(), t_nmi(), t_int3(), t_into(), t_check(),
	t_und(), t_dna(), t_syserr(), t_res(), t_badtss(), t_notpres(),
	t_stkflt(), t_gpflt(), t_pgflt(), t_coperr();
	
int	t_svc0(), t_svc1(), t_svc2(), t_svc3(), t_svc4(), t_svc5(), t_svc6();

int	bin0int(), bin1int(), bin2int(), bin3int(),
	bin4int(), bin5int(), bin6int(), bin7int();

#ifdef	FPA
int	t_fpa();
#endif	FPA

struct	idt_init {
	int	idt_desc;
	int	idt_type;
	int	(*idt_addr)();
	int	idt_dpl;
};

struct	idt_init idt_init[] = {
	/*	Desc #		Type		Address		Privilege */
	{	T_DIVERR,	TRAP_GATE_TYPE,	t_diverr,	DPL_KERNEL },
	{	T_DBG,		TRAP_GATE_TYPE,	t_dbg,		DPL_KERNEL },
	{	T_NMI,		INTR_GATE_TYPE,	t_nmi,		DPL_KERNEL },
	{	T_INT3,		TRAP_GATE_TYPE,	t_int3,		DPL_USER },
	{	T_INTO,		TRAP_GATE_TYPE,	t_into,		DPL_USER },
	{	T_CHECK,	TRAP_GATE_TYPE,	t_check,	DPL_USER },
	{	T_UND,		TRAP_GATE_TYPE,	t_und,		DPL_KERNEL },
	{	T_DNA,		TRAP_GATE_TYPE,	t_dna,		DPL_KERNEL },
	{	T_SYSERR,	INTR_GATE_TYPE,	t_syserr,	DPL_KERNEL },
	{	T_BADTSS,	INTR_GATE_TYPE,	t_badtss,	DPL_KERNEL },
	{	T_NOTPRES,	TRAP_GATE_TYPE,	t_notpres,	DPL_KERNEL },
	{	T_STKFLT,	TRAP_GATE_TYPE,	t_stkflt,	DPL_KERNEL },
	{	T_GPFLT,	TRAP_GATE_TYPE,	t_gpflt,	DPL_KERNEL },
	{	T_PGFLT,	TRAP_GATE_TYPE,	t_pgflt,	DPL_KERNEL },
	{	T_COPERR,	TRAP_GATE_TYPE,	t_coperr,	DPL_KERNEL },
	{	T_SVC0,		TRAP_GATE_TYPE,	t_svc0,		DPL_USER },
	{	T_SVC1,		TRAP_GATE_TYPE,	t_svc1,		DPL_USER },
	{	T_SVC2,		TRAP_GATE_TYPE,	t_svc2,		DPL_USER },
	{	T_SVC3,		TRAP_GATE_TYPE,	t_svc3,		DPL_USER },
	{	T_SVC4,		TRAP_GATE_TYPE,	t_svc4,		DPL_USER },
	{	T_SVC5,		TRAP_GATE_TYPE,	t_svc5,		DPL_USER },
	{	T_SVC6,		TRAP_GATE_TYPE,	t_svc6,		DPL_USER },
	/*
	 * SLIC Bin0-7 Interrupts.  All must be intrrupt-gate to avoid
	 * re-enter interrupt handler before ready to; SLIC yanks interrupt
	 * request line until we read the 8-bit vector number.
	 */
	{	T_BIN0,		INTR_GATE_TYPE,	bin0int,	DPL_KERNEL },
	{	T_BIN1,		INTR_GATE_TYPE,	bin1int,	DPL_KERNEL },
	{	T_BIN2,		INTR_GATE_TYPE,	bin2int,	DPL_KERNEL },
	{	T_BIN3,		INTR_GATE_TYPE,	bin3int,	DPL_KERNEL },
	{	T_BIN4,		INTR_GATE_TYPE,	bin4int,	DPL_KERNEL },
	{	T_BIN5,		INTR_GATE_TYPE,	bin5int,	DPL_KERNEL },
	{	T_BIN6,		INTR_GATE_TYPE,	bin6int,	DPL_KERNEL },
	{	T_BIN7,		INTR_GATE_TYPE,	bin7int,	DPL_KERNEL },
	/*
	 * Weitek FPA exception vector.
	 */
	{	T_FPA,		INTR_GATE_TYPE,	t_fpa,		DPL_KERNEL },
	{ 0 }
};


static
init_desc_tables(pl)
	register struct plocal *pl;
{
	register struct gate_desc *gd;
	register struct idt_init *id;

	/*
	 * Build GDT.  Use per-processor TSS to avoid problems with
	 * BUSY TSS when on-line processors.
	 */

	BUILD_SEG_DESC(&pl->gdt[GDT_KCS], 0, SD_MAX_SEG,
					CODE_SEG_TYPE, DPL_KERNEL);
	BUILD_SEG_DESC(&pl->gdt[GDT_KDS], 0, SD_MAX_SEG,
					DATA_SEG_TYPE, DPL_KERNEL);
	BUILD_SEG_DESC(&pl->gdt[GDT_UCS], VA_USER, SD_MAX_SEG,
					CODE_SEG_TYPE, DPL_USER);
	BUILD_SEG_DESC(&pl->gdt[GDT_UDS], VA_USER, SD_MAX_SEG,
					DATA_SEG_TYPE, DPL_USER);
	BUILD_TSS_DESC(&pl->gdt[GDT_TSS], &pl->tss, sizeof(struct tss_seg),
					DPL_KERNEL);

	/*
	 * Build IDT.  Initialize IDT to all "t_res", then
	 * fill out from table.
	 */

	for (gd = pl->idt; gd < &pl->idt[IDT_SIZE]; gd++)
		BUILD_GATE_DESC(gd, KERNEL_CS, t_res, INTR_GATE_TYPE, DPL_KERNEL, 0);

	for (id = idt_init; id->idt_addr != NULL; id++) {
		gd = &pl->idt[id->idt_desc];
		BUILD_GATE_DESC(gd, KERNEL_CS, id->idt_addr,
					id->idt_type, id->idt_dpl, 0);
	}

	/*
	 * Init GDTR and IDTR to start using new stuff.
	 */

	BUILD_TABLE_DESC(&desctab, pl->idt, IDT_SIZE);
	asm("lidt _desctab");

	BUILD_TABLE_DESC(&desctab, pl->gdt, GDT_SIZE);
	asm("lgdt _desctab");

	/*
	 * Fill out TSS so setup_seg_regs() can load the register.
	 * TSS was zeroed by selfinit() when whole struct plocal got zapped.
	 * Make I/O instructions generate a fault.
	 */

	pl->tss.ts_ss0 = KERNEL_DS;
	pl->tss.ts_esp0 = VA_UAREA + UPAGES*NBPG;
	pl->tss.ts_bmapoff = sizeof(struct tss_seg);

	/*
	 * Initialize the segment registers, TSS register, LDT register.
	 */

	setup_seg_regs();
}

/*
 * calloc()
 *	Allocate zeroed memory at boot time.
 *
 * Done via bumping "curmem" value.
 *
 * Skips holes in physical memory after memory is configured (topmem != 0).
 * Assumes allocations to that point are in memory contiguous from physical 0.
 *
 * callocrnd() is used to round up so that next allocation occurs
 * on a given boundary.
 */

caddr_t
calloc(size)
	int	size;
{
	caddr_t	val;

	ASSERT(calloc_ok, "calloc: too late");
        /*
         *+ The calloc function to allocate kernel memory was called too
         *+ late in system initialization.  After this point, other kernel
         *+ routines for memory allocation should have been used.
         */

	size = (size + (sizeof(int) - 1)) & ~(sizeof(int)-1);

	/*
	 * If ok to check, insure memory exists and skip hole if necessary.
	 * Skipping hole puts curmem on hole boundary, thus arbitrary alignment.
	 * Doing this in terms of CLBYTES is independent of memory bit-map
	 * grain size.
	 */

	if (topmem != 0) {
		while (!cmem_exists(curmem, size)) {
			curmem = (caddr_t) (((int)curmem + CLBYTES) & ~CLOFSET);
			ASSERT(curmem < topmem, "calloc: not enough memory");
                        /*
                         *+ The kernel tried to allocate memory and sufficient
                         *+ resources were not available.
                         */
		}
	}

	/*
	 * Allocate and clear the memory.
	 */

	val = (caddr_t) curmem;
	curmem += size;

	bzero(val, (unsigned)size);
	return(val);
}

callocrnd(bound)
	int	bound;
{
	curmem = (caddr_t)roundup((int)curmem, bound);
}

/*
 * cmem_exists()
 *	Check for existence of memory from a given address for a given size.
 */

static
cmem_exists(paddr, size)
	caddr_t	paddr;
	register int	size;
{
	register int	pg;

	size += (int) paddr & (NBPG-1);
	for (pg = btop(paddr); size > 0; size -= NBPG, pg++)
		if (!page_exists(pg))
			return(0);
	return(1);
}

/*
 * Return a number to use in spin loops that takes into account
 * both the cpu rate and the mip rating.
 * If the machine is not yet up, then use cpurate otherwise
 * use l.cpu_speed which will then be defined.
 */

calc_delay(x)
	unsigned int	x;
{
	extern short	upyet;
	extern int	cpurate;

	if (!upyet) {
		ASSERT_DEBUG(cpurate,"cpurate not set");
		return (x*cpurate);
	} else {
		ASSERT_DEBUG(l.cpu_speed,"l.cpu_speed not set");
		return (x*l.cpu_speed);
	}
}

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
static	char	rcsid[] = "$Header: autoconf.c 2.35 1991/08/07 21:24:28 $";
#endif

/*
 * autoconf.c
 *	Auto-configuration.
 */

/* $Log: autoconf.c,v $
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
#include "../h/cmn_err.h"
#include "../h/file.h"

#include "../balance/clock.h"
#include "../balance/engine.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/clkarb.h"
#include "../balance/cfg.h"

#include "../machine/ioconf.h"
#include "../machine/pte.h"
#include "../machine/hwparam.h"
#include "../machine/plocal.h"
#include "../machine/mmu.h"
#include "../machine/gate.h"
#include "../machine/intctl.h"

#ifdef i386
/*
 * The scan interface is supported on Model-D and Model-E i386 procs only.
 */
#define	SCAN	1
#endif /* i386 */

unsigned	Nengine;			/* # processors */
struct	engine	*engine;			/* base of engine array */
struct	engine	*engine_Nengine;		/* end of engine array */
int		NFPA = 0;			/* # processors with FPA's */
int		mono_P_slic = -1;		/* no mono_P drivers == -1 */
short		fp_lights;			/* Front panel lights */
u_char          cons_slic = 0xff;               /* console device's SLIC id */
u_char		cons_scsi = 0xff;		/* console SCSI's SLIC id */

unsigned	boothowto;			/* boot flags */
unsigned	sys_clock_rate;			/* # Mega Hz system runs at */

struct	bin_header int_bin_table[SLICBINS];	/* Interrupt Bin Table */
int	bin_alloc[SLICBINS];			/* for allocating vectors */

/*
 * slic_to_config[] maps SLIC number to configuration information about
 * that SLIC.
 */

struct	ctlr_desc *slic_to_config[MAX_NUM_SLIC];

/*
 * sec0eaddr encodes a system unique 24-bit number.  Name is historic.
 * This value is the low-order 24-bits of the ether address on SCED[0] for
 * a B8k; on a B21k it's the system-ID value from the backplane (readable thru
 * the CADM).  After system is up and /etc/init has the magic system ID number,
 * sec0eaddr holds the number of users the system can legally support.
 */

unsigned	sec0eaddr = -1;		/* system ID number */

caddr_t		calloc();
int		strayint();

/*
 * configure()
 *	Scan the HW configuration information, do probes, etc,
 *	all in the name of determining what's out there.
 */

configure()
{
	register struct ctlr_desc *cd;
	register struct cntlrs	*b8k;

	/*
	 * Determine boot flags and system clock rate.
	 */

	boothowto = CD_LOC->c_boot_flag;
	sys_clock_rate = CD_LOC->c_clock_rate;

	/*
	 * Build slic to config information map.
	 */

	for (cd = CD_LOC->c_ctlrs; cd < CD_LOC->c_end_ctlrs; cd++)
		slic_to_config[cd->cd_slic] = cd;

	/*
	 * Do configuration/allocation of basic system components.
	 */

	conf_console();
	conf_clkarb();
	conf_proc();
	conf_mem();

	for (b8k = b8k_cntlrs; b8k->conf_b8k != NULL; b8k++)
		(*b8k->conf_b8k)();

	/*
	 * Allocate interrupt table, set up pseudo devices, and
	 * probe IO controllers (includes MBAd's, SCED's, ZDC's, etc).
	 */

	conf_intr();
	conf_pseudo();

	for (b8k = b8k_cntlrs; b8k->conf_b8k != NULL; b8k++)
		(*b8k->probe_b8k_devs)();
	
	/*
	 * Allow GENERIC configure to set root and swap configuration.
	 */
	setconf();

}
/*
 * conf_console()
 *	Determine the system controller and set up
 *	to handle its misc. functions.  These include
 *	console printf, controller-memory driver, and
 * 	console driver.
 */

static
conf_console()
{
	register struct cdevsw *cdev;
	register int i;
	extern int smem_major;		/* Defined in balance/smem.c */
	extern int cons_major;		/* Defined in balance/cons.c */
	extern smopen(), coopen(); 	/* SCED dependent driver interfaces */
	extern ssopen(), scopen(); 	/* SSM dependent driver interfaces */

	switch (CD_LOC->c_cons->cd_type) {
	case SLB_SCSIBOARD:
		cons_slic = cons_scsi = CD_LOC->c_cons->cd_slic;
		for (i = 0, cdev = cdevsw; i < nchrdev; i++, cdev++) {
			if (cdev->d_open == smopen) 
				smem_major = i;
			else if (cdev->d_open == coopen) 
				cons_major = i;
		}
		break;
	case SLB_SSMBOARD:
	case SLB_SSM2BOARD:
		cons_slic = cons_scsi = CD_LOC->c_cons->cd_slic;
		for (i = 0, cdev = cdevsw; i < nchrdev; i++, cdev++) {
			if (cdev->d_open == ssopen) 
				smem_major = i;
			else if (cdev->d_open == scopen) 
				cons_major = i;
		}
		break;
	default:
		panic("Unrecognizable system controller");
		/*
		 *+ The system configuration information identifies the
		 *+ system controller hardware as an unknown device.
		 *+ Power-up software did not properly identify the board.
		 */
		break;
	}
}

/*
 * conf_clkarb()
 *	Determine if clock arbiter is present. If present,
 *	set flag for front panel lights. And determine bus priority
 *	for slots 16-20. If all are processors then set priority to low
 *	otherwise set high (default).
 */

#ifdef	ns32000
/*
 * Data-mover only used in 032 systems.
 */
u_char	cadm_hrev_data_mover = 1;
extern	bool_t	use_data_mover;
#endif	ns32000

static
conf_clkarb()
{
	register struct ctlr_toc *toc = &CD_LOC->c_toc[SLB_CLKARBBOARD];
	register struct ctlr_desc *cd;
	extern	int	light_show;
	extern	lock_t	diskoff_lock;

	init_lock(&diskoff_lock, G_DKOFFLINE);	/* mutex # offline disks cnt */

	if (toc->ct_count == 0) {
#ifdef	ns32000
		use_data_mover = 0;		/* not gots one! */
#endif	ns32000
		return;
	}

	/*
	 * We're a B21k ==> set up to use front-panel LED's and get system ID
	 * from clock-arbiter board.
	 */

	cd = &CD_LOC->c_ctlrs[toc->ct_start];		/* clkarb ctlr_desc */

	fp_lights = 1;			/* use front panel LEDs */
	if (light_show > 1)
		fp_lights = -1;		/* use front-panel and processor LEDs */
	FP_IO_ONLINE;			/* ASSUME all drives online */

	sec0eaddr = cd->cd_ca_sysid & 0x00FFFFFF;

#ifdef	ns32000
	/*
	 * If CADM isn't late enough rev, don't use the data-mover.
	 * Also don't use data-mover if it failed any diagnostic.
	 */

	if (cd->cd_hrev < cadm_hrev_data_mover ||
	    (cd->cd_diag_flag & CFG_C_BAD_DM))
		use_data_mover = 0;

	/*
	 * If running on old SCED firmware, firmware didn't adjust
	 * b21k bus priority if all processors in high priority slots.
	 * If anything in a high priority bus slot needs the high priority,
	 * leave settings alone, else set lower priority.
	 *
	 * This is only necessary for 032 systems, since SGS systems run
	 * on new SCED FW.
	 */

	for (cd = CD_LOC->c_ctlrs; cd < CD_LOC->c_end_ctlrs; cd++) {
		if ((cd->cd_flags & CTF_HIPRI)
		&&  cd->cd_slic >= CA_FOL && cd->cd_slic <= CA_LOL)
			return;
	}
	CPRINTF("Setting optional priority slots to LOW priority\n");
	wrslave(CLKARB_SLIC, SL_C_DIAG_CTRL, SLB_OPT_PRI | SLB_EN_BE);
#endif	ns32000
}

/*
 * conf_proc()
 *	Configure processors.
 *
 * Allocate engine table for all possible processors, but only remember
 * alive and configured processors.
 *
 * We only fill out the slic addresses in the engine structures;
 * sysinit() fills out the rest.
 *
 * We also set `Nengine' here to the # of desired processors.
 */

#ifdef	ns32000
int	min_proc_rev = 0x7fffffff;	/* minimum processor board HW rev */
#endif	ns32000

/*
 * This is a general hint to the SSM handler whether there are
 * SGS2 processor boards present (i.e. if the interrupting version of the
 * SSM firmware has been installed).
 */
int     nsgs2 = 0;


static
conf_proc()
{
	extern	int	cpurate;		/* calculated value */
	extern	int	lcpuspeed;		/* configured value */
	extern	int	i486_lcpuspeed;		/* configured value */
	register struct ctlr_toc *toc;
	register struct	ctlr_desc *cd;
	register struct engine *eng;
	register int i;
	int 	proc_cnt;
	int	sgs_cnt;
	int	flags = 0;

	/*
	 * Get table of contents pointer for processor board.
	 */

#ifdef	ns32000
	toc = &CD_LOC->c_toc[SLB_PROCBOARD];		/* 032 processors */
	proc_cnt = toc->ct_count;
#endif	ns32000

#ifdef	i386
      	/*
	 * Determine the number of processors on the system.
	 * For the i386 family, there can be two types:  SGS (modelB, modelC)
	 * and SGS2 (modelD, modelE).
	 */
	 nsgs2 = CD_LOC->c_toc[SLB_SGS2PROCBOARD].ct_count;  
							/* SGS2 processors */

	 toc = &CD_LOC->c_toc[SLB_SGSPROCBOARD];         /* SGS processors */
	 sgs_cnt = toc->ct_count;
	 proc_cnt = nsgs2 + sgs_cnt;
#endif	i386

	engine = (struct engine *)calloc(proc_cnt * sizeof(struct engine));
	CPRINTF("%d processors; slic", proc_cnt);
again:
	cd = &CD_LOC->c_ctlrs[toc->ct_start];
	for (i = 0; i < toc->ct_count; i++, cd++) {
		CPRINTF(" %d", cd->cd_slic);
		if (cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF))
			continue;
		eng = &engine[Nengine++];
		eng->e_diag_flag = cd->cd_diag_flag;
		eng->e_slicaddr = cd->cd_slic;
		eng->e_cpu_speed = cd->cd_p_speed;
		/*
		 * Set the engine rate and find the fastest proccesor
		 * if not set assume maximum.
		 * Note: cpurate is recalulated to be in MIPS at the
		 * end of this routine.
		 */
		eng->e_cpu_speed = cd->cd_p_speed;
		eng->e_flags = flags;
		if (eng->e_cpu_speed == 0 )
			eng->e_cpu_speed = cpurate;
		else if (eng->e_cpu_speed > cpurate)
			cpurate = eng->e_cpu_speed;
#ifdef	E_FPU387
		/*
		 * Set E_FPU387 and E_FPA in e_flags as appropriate.
		 * Bump NFPA for those available processors that have FPA's.
		 * Only set E_FPA if there is an FPA and it passed diagnostics.
		 */
		if (cd->cd_p_fp & SLP_387)
			eng->e_flags |= E_FPU387;
		if (cd->cd_p_fp & SLP_FPA) {
			if ((cd->cd_diag_flag & CFG_SP_FPA) == 0) {
				eng->e_flags |= E_FPA;
				++NFPA;
			}
		}
#endif	E_FPU387
#ifdef	ns32000
		/*
		 * Keep track of minimum processor board rev, so certain
		 * 032 chip-bug work-arounds can be backed out if processor
		 * boards have late enough revision chips.
		 */
		if (cd->cd_hrev < min_proc_rev)
			min_proc_rev = cd->cd_hrev;
#endif	ns32000
	}
	if (flags == 0) {
		/*
		 * Go get all SGS2 processors.
		 */
		flags = E_SGS2 | E_FPU387;
		toc = &CD_LOC->c_toc[SLB_SGS2PROCBOARD];
		goto again;
	}
	engine_Nengine = &engine[Nengine];
	CPRINTF(".\n");

	if (Nengine < proc_cnt) {
		CPRINTF("Not using processors: slic");
#ifdef ns32000
		toc = &CD_LOC->c_toc[SLB_PROCBOARD];
		cd = &CD_LOC->c_ctlrs[toc->ct_start];
#else
		toc = &CD_LOC->c_toc[SLB_SGSPROCBOARD];
		cd = &CD_LOC->c_ctlrs[toc->ct_start];	
#endif ns32000

		for (i = 0; i < proc_cnt; i++, cd++) {
#ifdef i386
			/*
			 * Look at SGS2 boards after exhausing
			 * SGS boards (SGS2 boards have i486 processors).
			 */
			if (i == sgs_cnt) {
				toc = &CD_LOC->c_toc[SLB_SGS2PROCBOARD];
				cd = &CD_LOC->c_ctlrs[toc->ct_start];
			}
#endif i386
			if (cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF))
				CPRINTF(" %d", cd->cd_slic);
		}
		CPRINTF(".\n");
	}
	/*
	 * compute cpurate for delay loops. The vaule should 
	 * correspond to the "mips" rating for the processor 
	 * at its running rate. 
	 * cpurate: is an estimate of the actual boards running rate 
	 * (usualy max)
	 * lcpuspeed: is the number of mips if sys_clock_rate = 100
	 * cpurate is only used for probe routines running on the
	 * "boot" processor.
	 * Note this gets replaced in selfinit() with l.cpu_speed.
	 * when the actual cpu rate is know.
	 */
#ifdef i386
	if (nsgs2 > 0)
		lcpuspeed = i486_lcpuspeed;
#endif i386
	cpurate = (lcpuspeed * cpurate) / 100;
}

/*
 * conf_intr()
 *	Allocate and initialize interrupt vector table.
 *
 * Sysinit() already set up master HW vector table.
 * Don't allocate anything for bin[0] (SW -- doesn't use this table).
 */

static
conf_intr()
{
	register int i;
	register int vec;
	extern	todclock();		/* tod clock handler */
#ifdef SCAN
	extern int  setup_scan_intr;
#endif /* SCAN */

	/*
	 * Add in local clock, tod clock to appropriate bins.
	 */
	ivecres(LCLKBIN, 1);
	ivecres(TODCLKBIN, 1);

	/*
	 * Allocate int_bin_table, init all entries to point at strayint().
	 */

	for (i = 1; i < SLICBINS; i++) {
		if (bin_intr[i] == 0)
			continue;
		ASSERT(bin_intr[i] <= MSGSPERBIN, "conf_intr: bin overflow");
		/*
		 *+ Too many intterupts have been declared for one
		 *+ of the interrupt bins. Check the value of MSGSPERBIN
		 *+ against all configured bin interrupts.
		 */
		int_bin_table[i].bh_size = bin_intr[i];
		int_bin_table[i].bh_hdlrtab = (int(**)())calloc(bin_intr[i]*sizeof(int (*)()));
		for (vec = 0; vec < int_bin_table[i].bh_size; vec++)
			ivecinit(i, vec, strayint);
	}

	/*
	 * Set up vectors for local clock and tod clock.
	 * Must do LCLKBIN first, to insure it gets vector 0.
	 */

#ifdef  ns32000
	/*
	 * 032 kernel vectors to clkint() (machine/locore.s) which
	 * arranges the stack for a call to hardclock().  This should
	 * eventually be modified to a direct call to hardclock() ala SGS.
	*/
	{
		extern  clkint();
		ivecinit(LCLKBIN, ivecall(LCLKBIN), clkint);
	}
#endif  ns32000
#ifdef	i386
	/*
	 * i386 kernel vectors directly to hardclock().
	 */
	{
		extern	hardclock();
		ivecinit(LCLKBIN, ivecall(LCLKBIN), hardclock);
	}
#endif	i386
	ivecinit(TODCLKBIN, ivecall(TODCLKBIN), todclock);

#ifdef SCAN
	if (setup_scan_intr) {
		extern ssm_misc_intr();
		ivecinit(7, ivecall(7), ssm_misc_intr);
	}
#endif /* SCAN */
}

/*
 * conf_pseudo()
 *	Call the boot procedures of pseudo-devices.
 */

static
conf_pseudo()
{
	register struct pseudo_dev *pd;

	CPRINTF("Pseudo devices:");
	for (pd = pseudo_dev; pd->pd_name; pd++) {
		CPRINTF(" %d %s", pd->pd_flags, pd->pd_name);
		(*pd->pd_boot)(pd->pd_flags);
	}
	CPRINTF(".\n");
}

/*
 * ivecall()
 *	Allocate a vector from a given bin.
 *
 * Insures sequential values returned per bin.
 */

u_char
ivecall(bin)
	u_char	bin;
{
	if (bin_alloc[bin] >= int_bin_table[bin].bh_size) {
		printf("Too many vectors in bin %d.\n", bin);
		panic("ivecall");
                /*
                 *+ Too many vectors were allocated from the given bin.  
		 *+ Corrective action: reconfigure the system so that fewer
		 *+ SLIC interrupt vectors are allocated from the 
		 *+ referenced bin.
                 */

		/*NOTREACHED*/
	}
	return(bin_alloc[bin]++);
}

/*
 * strayint()
 *	Stray interrupt catcher.
 *
 * Doesn't report bin #; instead reports current value of SLIC local
 * mask which allows inference of interrupting bin.
 */

static
strayint(vec)
	int	vec;			/* vector number within bin */
{
	printf("Stray intr, vector %d ipl 0x%x.\n", vec, va_slic->sl_lmask);
        /*
         *+ A stray interrupt was caught.
         *+ The interrupt occurred at a level not expected by the system.
         */
}

/*
 * bogusint()
 *	Called from locore.s when bad vector number presented
 *	from SLIC.
 */

bogusint(bin, vec)
	unsigned bin;
	unsigned vec;
{
	printf("Bogus interrupt vector %d on bin %d.\n", vec, bin);
        /*
         *+ A bad vector number has been passed with an intterupt.
         */
}

/*
 * swapconf()
 *	Configure swap space and related parameters.
 */

swapconf()
{
	register struct swdevt *swp;
	register int nblks;
	int mode;

	/*
	 * For each swap partition, call driver to establish real
	 * size.  If driver returns -1 (or configuration has this)
	 * then complain on console, and mark "freed" so swapon()
	 * won't see it.  If partition 0 is bad, panic; this is
	 * required for args and some swap space.
	 */

	if (swdevt[0].sw_dev == rootdev) {
		mode = FSPEC;	/* overide usage checking for minroot */
	} else {
		mode = FSWAP;
	}
	for (swp = swdevt; swp->sw_dev; swp++) {
		nblks = -1;
		/*
		 * open to force VTOC access
		 */
		if ((*bdevsw[major(swp->sw_dev)].d_open)(swp->sw_dev, mode) == 0) {
			if (bdevsw[major(swp->sw_dev)].d_psize) {
				nblks =
			     (*bdevsw[major(swp->sw_dev)].d_psize)(swp->sw_dev);
			}
			if (bdevsw[major(swp->sw_dev)].d_close)
				(*bdevsw[major(swp->sw_dev)].d_close)(swp->sw_dev, mode);
		}
		if (swp->sw_nblks == 0 || swp->sw_nblks > nblks)
			swp->sw_nblks = nblks;
		if (swp->sw_nblks < 0) {
			if (swp > swdevt)
				printf("WARNING: ");
			printf("Swap partition[%d], dev=(%d,%d) does not exist.\n",
				swp-swdevt, major(swp->sw_dev), minor(swp->sw_dev));
			/*
			 *+ A swap partition specified in the kernel
			 *+ configuration file is not present.
			 *+ The system will disable swapping to that partition.
			 */
			swp->sw_freed = 1;
			if (swp == swdevt) {
				panic("swapconf: no arg space");
				/*
				 *+ No swap devices can be found.
				 *+ The system is unable to cintinue.
				 *+ Check your kernel configurations files
				 *+ or check that the swap disk are spun up.
				 */
			}
		}
	}

	/*
	 * Set up dmap parameters (swap space representation objects).
	 */

	init_dmap();
}

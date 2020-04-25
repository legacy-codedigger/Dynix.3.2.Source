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
static	char	rcsid[] = "$Header: mbad.c 2.28 1991/06/11 22:39:40 $";
#endif

/*
 * mbad.c
 *	Various procedures dealing with MBIf's.
 */

/* $Log: mbad.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/clist.h"
#include "../h/buf.h"
#include "../h/vnode.h"
#include "../h/cmn_err.h"

#include "../balance/cfg.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/bic.h"

#include "../machine/ioconf.h"
#include "../machine/hwparam.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"
#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../machine/trap.h"
#include "../machine/plocal.h"

#include "../mbad/ioconf.h"
#include "../mbad/mbad.h"

#ifdef	DEBUG
int	mbad_debug = 0;		/* get verbose flag */
#endif	DEBUG

struct mb_desc	*MBAd;		/* base of array */
int	 	NMBAd;		/* # MBAd's to map at boot */
u_int	 	MBAdvec;	/* bit-vector of existant MBAd's */
u_char		MBAd_errbase;	/* MBAd error base vector */
struct pte	*MBIOmap;	/* mapping pages for MBAd's */

int		nclistmap;	/* # MB map registers for clist */

struct mb_desc	*MBAd_probe();	/* MBAd device prober */
int		MBAd_error();	/* MBAd error catcher */
int		MBAd_nmi();	/* handler for NMI during probe */
label_t		MBAd_nmi_jmp;	/* for NMI recovery during probe */
int		MBAd_idx;	/* index of mbad being probed for devices */

#define	MBAD_CLEAR(idx)		MBAdvec &= ~(1 << (idx))
#define	MBAD_EXISTS(idx)	(MBAdvec & (1 << (idx)))
#define	WILDCARD(md)		((md)->md_idx == -1)

#define	PLURAL(x)		((x)==1?"":"s")

/*
 * conf_mbad()
 *	Allocate and fill out MBAd descriptors.
 *
 * Can deal with deconfigured boards (uses MBAdvec).
 *
 * We fill out all the MBAd[*]'s, using MBAdvec to tell sysinit()
 * which ones to map.
 *
 * Must be careful to leave enough DMA mapping registers to map
 * the clist in each MBAd.  `nclist' holds # clist entries that
 * will be allocated; use this to compute # mapping registers needed
 * and allocate them at the top.  C-list isn't allocated yet; thus
 * can only allocate the space.  Sysinit() will fill out the mb_clist
 * field.
 */

#ifdef	ns32000
u_long	virt_mbad;			/* kernel base virt address of MBAd's */
extern	u_long	max_ker_vaddr;		/* max kernel virtual address */
#endif	ns32000

conf_mbad()
{
	register struct ctlr_toc *toc = &CD_LOC->c_toc[SLB_MBABOARD];
	register struct	ctlr_desc *cd;
	register struct mb_desc *mb;
	register int i;

	nclistmap = 1 + howmany(nclist * sizeof(struct cblock), MB_MRSIZE);

	MBAd = (struct mb_desc *)calloc((int)toc->ct_count * sizeof(struct mb_desc));

	if (toc->ct_count) {
#ifdef	ns32000
		/*
		 * Move MBAd mapping.  Power-up/boot firmware placed the MBAd's
		 * starting at 8Meg.  Must move before do any probes/boots since
		 * some driver save MBAd addresses.
		 *
		 * Have MBAd mapping top out at max_ker_vaddr, and adjust
		 * max_ker_vaddr to reflect this).
		 *
		 * Use ovbcopy() since source may overlap destination.
		 */
		virt_mbad = max_ker_vaddr - toc->ct_count * MBAD_ADDR_SPACE;
		ovbcopy((caddr_t) (CD_LOC->c_pagetable + L1IDX(OLD_VIRT_MBAD)),
			(caddr_t) (CD_LOC->c_pagetable + L1IDX(virt_mbad)),
			(unsigned) (toc->ct_count
					* ((MBAD_ADDR_SPACE/NBPG)/NPTEPG))
					* sizeof (struct pte)
		);
		max_ker_vaddr = virt_mbad;
#endif	ns32000
		CPRINTF("%d MULTIBUS Adapter%s; slic",
				toc->ct_count, PLURAL(toc->ct_count));
		/*
		 *+ Informational only.
		 */
	} else {
		CPRINTF("No MULTIBUS Adapters");
	}

	cd = &CD_LOC->c_ctlrs[toc->ct_start];
	for (i = 0; i < toc->ct_count; i++, cd++) {
		mb = &MBAd[i];
		mb->mb_slicaddr = cd->cd_slic;
		mb->mb_mem = (caddr_t)VA_MBAd(i);
		mb->mb_ios = (struct mb_ios *)(VA_MBAd(i) + MBAd_IOwindow);
		mb->mb_dmabase = MBAd_IOwindow;
		if ((cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF)) == 0) {
			++NMBAd;
			MBAdvec |= 1 << i;
		}
		CPRINTF(" %d", mb->mb_slicaddr);
	}
	CPRINTF(".\n");

	/*
	 * Allocate MBAd error interrupt vectors.
	 * Must be aligned and allocated contiguously over all MBAds.
	 */

	ivecres(MBAD_ERROR_BIN, (toc->ct_count * MBAD_ERR_ALIGN) + MBAD_ERR_ALIGN);

	if (NMBAd < toc->ct_count) {
		CPRINTF("Not using MULTIBUS Adapters: slic");
		for (i = 0; i < toc->ct_count; i++) {
			if (!MBAD_EXISTS(i))
				CPRINTF(" %d", MBAd[i].mb_slicaddr);
		}
		CPRINTF(".\n");
	}
}

/*
 * probe_mbad_devices()
 *	Probe for devices attached to MBAd's.
 *
 * Also, init error handling vectors per MBAd.
 *
 * Allocate enough level-2 mapping pages to map all possible MBAd's
 * (4K page maps 4Meg, so typically one mapping page).
 */

probe_mbad_devices()
{
	struct ctlr_toc *toc = &CD_LOC->c_toc[SLB_MBABOARD];
	register struct	mbad_conf *mc;
	register struct mbad_driver *mbd;
	register struct	mbad_dev *md;
	register struct	mb_desc	*mb;
	register int i;
	u_char	vec;
	int	maps;
	int	ndev = 0;
	int	bits;
	extern	int mono_P_slic;

	/*
	 * Initialize MBAd error reporting and mapping.
	 */

	CPRINTF("MBAd DMA window at 0x%x.\n", MBAd_IOwindow);

	bin_alloc[MBAD_ERROR_BIN] =
		roundup(bin_alloc[MBAD_ERROR_BIN], MBAD_ERR_ALIGN);
	MBAd_errbase = ivecpeek(MBAD_ERROR_BIN);

	for (i = 0; i < toc->ct_count; i++) {
		/*
		 * This relies on ivecall() returning consecutive vectors.
		 */
		vec = ivecall(MBAD_ERROR_BIN);
		ivecinit(MBAD_ERROR_BIN, vec, MBAd_error);
		if (!MBAD_EXISTS(i))
			continue;			/* deconf'd */
		MBAd_init(&MBAd[i], MBAD_ERROR_BIN, vec);

		bin_alloc[MBAD_ERROR_BIN] =
			roundup(bin_alloc[MBAD_ERROR_BIN], MBAD_ERR_ALIGN);
	}

	/*
	 * Run thru the mbad_conf array and do the probes.
	 * We call the boot procedure even if no existing HW is found.
	 *
	 * For each configured MBAd driver, for each board...
	 */

	for (mc = mbad_conf; mc->mc_driver; mc++) {
		mbd = mc->mc_driver;
		for (md = mc->mc_dev, i = 0; i < mc->mc_nent; i++, md++) {

			/*
			 * If it has an interrupt procedure, allocate a vector
			 * in the appropriate bin.  Must allocate even if
			 * probe ==> not here, to preserve order of devices.
			 */

			if (mbd->mbd_cflags & MBD_HASINTR) {
				vec = ivecall(md->md_bin);
				md->md_vector = vec;
			}

			/*
			 * If it's there, fill out configure fields in `md'
			 * and init MBAd interrupt line.  If can't allocate
			 * the map registers it needs, assume it doesn't
			 * exist.
			 */
			if (mb = MBAd_probe(mbd, md)) {
				++ndev;

				/*
				 * Allocate whatever map registers it needs.
				 * If can't fit, don't configure the device.
				 */
				maps = MBAd_mapall(mb, (int)md->md_mapwant);
				if (maps < 0) {
					printf("%s%d at MBAd%d csr 0x%x: can't fit %d map registers.\n",
						mbd->mbd_name, i, mb-MBAd,
						md->md_csr, md->md_mapwant);

					/*
					 *+ There are not enough map registers
					 *+ left on the named mbad to fill the
					 *+ needs of the named device.
					 *+ Corrective action:
					 *+ either configure this device with 
					 *+ more segments or a larger segsize, 
					 *+ or delete some device(s) from
					 *+ this mbad, as it can provide only
					 *+ a total of 256 DMA maps spread 
					 *+ among all attached devices.
					 */
					continue;

				}

				md->md_desc = mb;
				md->md_basemap = maps;
				md->md_nmaps = md->md_mapwant;

				if (mbd->mbd_cflags & MBD_MONOP) {
					mono_P_slic = va_slic->sl_procid;
				}

				/*
				 * If there is an interrupt, set up MBAd
				 * to respond properly to it.  Also,
				 * check that the same MBAd interrupt
				 * level isn't already in use.
				 */
				if (mbd->mbd_cflags & MBD_HASINTR) {
					if (mb->mb_intrs & (1 << md->md_level)) {
						printf("%s%d at MBAd%d csr 0x%x: MB level %d overlaps previous use.\n",
							mbd->mbd_name, i, mb-MBAd,
							md->md_csr, md->md_level);
						/*
						 *+ The named device has a 
						 *+ MULTIBUS interrupt level 
						 *+ that is the same as another
						 *+ already booted device on 
						 *+ the same MULTIBUS.  This is
						 *+ not allowed. Corrective 
						 *+ action: reconfigure the 
						 *+ device with a different 
						 *+ interrupt (specified with 
						 *+ unit number).
						 */
						(void) MBAd_mapall(mb, (int)(-md->md_nmaps));
						continue;
					}
					md->md_vector = vec;
					ivecinit(md->md_bin,vec,mbd->mbd_intr);

					/*
					 * Tell MBAd how to interrupt us.
					 * For mono-P driver, aim it at
					 * "self" (eg, boot processor (we
					 * know it exists!)), else make it
					 * a TMPOS group interrupt.
					 */
					MBAD_CMD(mb, MCMD_INIT_LINE|md->md_level);
					if (mbd->mbd_cflags & MBD_MONOP)
						MBAD_CMD(mb, mono_P_slic);
					else
						MBAD_CMD(mb, SL_GROUP|TMPOS_GROUP);
					MBAD_CMD(mb, vec);			/* message data */
					MBAD_CMD(mb, SL_MINTR|md->md_bin);	/* command */
					mb->mb_intrs |= 1 << md->md_level;
				}

				/*
				 * If it uses c-list, set flag in `mb' structure
				 * to indicate to mbad_map() (later) to map in
				 * the c-list.
				 */

				if (mbd->mbd_cflags & MBD_CLIST)
					mb->mb_clist = -1;

				/*
				 * Tell all about it...
				 */

				md->md_alive = 1;

				CPRINTF("%s%d found at MBAd%d csr 0x%x",
						mbd->mbd_name, i, mb-MBAd, md->md_csr);
				if (mbd->mbd_cflags & MBD_HASINTR)
					CPRINTF(", bin %d vec %d MB level %d",
						md->md_bin, md->md_vector, md->md_level);
				if (md->md_nmaps)
					CPRINTF(", %d maps at %d",
						md->md_nmaps, md->md_basemap);
				CPRINTF(".\n");
			}
		}

		/*
		 * Probes done, "boot" the driver.
		 */
		if (mbd->mbd_cflags & MBD_HASBOOT)
			(*mbd->mbd_boot)(mc->mc_nent, mc->mc_dev);
	}

	/*
	 * Try to insure C-list fits in all MBAd's that need it; impose
	 * a minimum of 100 entries (consistent with conf/param.c).
	 * If this happens often, need to modify drivers to not need
	 * C-list mapped 1-1 in limited MBAd mapping space.
	 *
	 * Non-existent MBAd's don't indicate need for clist mapping.
	 * This relies on sizeof(struct cblock) dividing MB_MRSIZE.
	 * C-list has not yet been allocated (see sysinit()).
	 */

	for (mb = &MBAd[0], bits = MBAdvec; bits != 0; bits >>= 1, ++mb) {
		if (mb->mb_clist && (mb->mb_dmareg + nclistmap) > MB_MAPS) {
			int	extra = mb->mb_dmareg + nclistmap - MB_MAPS;
			nclistmap -= extra;
			nclist -= extra * (MB_MRSIZE / sizeof(struct cblock));
			printf("WARNING: Dropping %d c-list entries to fit in MBAd%d; nclist = %d.\n",
				extra * (MB_MRSIZE / sizeof(struct cblock)),
				mb - &MBAd[0], nclist);
			/*
 			 *+ More clists have been allocated than can be mapped
			 *+ by the the multibuss adpater. The number of clists
			 *+ have been reduced.
			 */
			ASSERT(nclist >= 100, "probe_mbad_devices: nclist too small");
			/*
			 *+ After attempting to reduce the number of clist
			 *+ entries for MBAD sizeing, less than 100 clist
			 *+ remain. The value of NCLIST should be inspected.
			 */
		}
	}
	/*
	 * If any devices, allocate mapping pages for them.
	 */

	if (ndev) {
		callocrnd(NBPG);
#ifdef	i386
		/*
		 * Must allocate for HW # of MBAd's since several MBAd's
		 * may be mapped by same page.
		 */
		MBIOmap = (struct pte *) calloc(
			howmany((int)toc->ct_count*MBAD_ADDR_SPACE, NPTEPG*NBPG)
			* NBPG);
#endif	i386
#ifdef	ns32000
		/*
		 * Need only allocate ptes for existing MBAd's, since
		 * single page of mapping maps subset of MBAd.
		 */
		MBIOmap = (struct pte *) calloc(NMBAd * NPTEMBAD * sizeof(struct pte));
#endif	ns32000
	}
}

/*
 * MBAd_init()
 *	Init MBAd by setting up error vectors and enable mapping registers.
 *
 * Does not set up individual multi-bus interrupt vectors.
 * Called by probe_mbad_devices() very early in init (prior to probing
 * MBAd devices), with only one processor alive and interrupts off.
 */

static
MBAd_init(mb, bin, vecbase)
	register struct mb_desc *mb;
	u_char	bin;			/* bin to take interrupts from MBAd */
	u_char	vecbase;		/* vector from `bin' for 1st MBAd error */
{
	register int i;
	u_char	MBAd_a_csr;

	/*
	 * Arrange that maskable interrupt to argument bin with argument
	 * vector is the result of nasty MBAd errors.
	 */

	MBAD_CMD(mb, MCMD_INIT_ERRORS);		/* next three cmds are... */
	MBAD_CMD(mb, SL_GROUP|TMPOS_GROUP);	/* destination,		  */
	MBAD_CMD(mb, vecbase);			/* message data,	  */
	MBAD_CMD(mb, SL_MINTR|bin);		/* command		  */

	/*
	 * Set up the IO/DMA space in the MBAd.
	 * We use the values placed in the MBAd's address csr by the
	 * power-up firmware, and modify the IO/DMA space window.
	 */

	MBAd_a_csr = rdslave(mb->mb_slicaddr, SL_A_CSR);
	MBAd_a_csr &= ~(SLB_S0|SLB_S1);
	MBAd_a_csr |= MBAd_IOwindow / (256*1024);
	wrslave(mb->mb_slicaddr, SL_A_CSR, MBAd_a_csr);

	/*
	 * Initialize mapping registers to 1-1 mapping and enable
	 * MBAd to use mapping registers.  1-1 mapping is necessary
	 * for debug and clean initial condition.
	 *
	 * Need to do this early to allow drivers to use mapping in
	 * boot/probe procedures.
	 */

	for(i = 0; i < MB_MAPS; i++)
		MBADMAP(mb, i) = i;

	mb->mb_ios->mb_ctl[0] = MBC_MAPENAB;		/* ON mapping */
}

/*
 * mbad_map()
 *	Fill out page-tables for existant MBAd's.  Also arrange that each
 *	MBAd map the c-list.  Called by sysinit() at initialization time.
 *
 * Assumes configure() filled out MBAdvec and allocated mapping pages (MBIOmap).
 *
 * C-list was allocated phys==virt, contiguous, in sysinit().
 */

mbad_map(kl1pt)
	struct pte *kl1pt;			/* kernel level1 page-table */
{
	register struct pte *pte;
	register unsigned paddr;
	register int i;
	struct	mb_desc *mb;
	int	clist_map;
	u_int	bits;

	/*
	 * If no devices found, then no map to create.
	 * Otherwise map the existing MBAd's.  De-conf'd
	 * MBAd's are left unmapped (0'd pte's).
	 */

	if (MBIOmap == NULL)
		return;

	pte = MBIOmap;
	for (i = 0, bits = MBAdvec; bits != 0; bits >>= 1, ++i) {
#ifdef	ns32000
		register int addr;
		register int j;
		int	l1idx;

		if (!MBAD_EXISTS(i))
			continue;
		l1idx = L1IDX(VA_MBAd(i));
		addr = 0;
		while(addr < MBAD_ADDR_SPACE) {
			ASSERT_DEBUG(*(int*)(&kl1pt[l1idx]) == 0,"kl1pt: MBAd");
			ASSERT_DEBUG(((int)pte & (NBPG-1)) == 0, "MBAd: align");
			*(int*)(&kl1pt[l1idx]) = (int)pte | PG_V|PG_R|PG_M | PG_KW;
			for (j = 0; j < NPTEPG; j++) {
				paddr = PA_MBAd(i)+addr;
				*(int*)(pte++) = PHYSTOPTE(paddr) | PG_V|PG_R|PG_M | PG_KW;
				addr += NBPG;
			}
			++l1idx;
		}
#endif	ns32000

#ifdef	i386
		if (!MBAD_EXISTS(i)) {
			pte += MBAD_ADDR_SPACE/NBPG;
			continue;
		}

		/*
		 * Insure level-1 maps it.  This is redundant except
		 * on NPTEPG*NBPG (4Meg) boundary.
		 */

		*(int *) &kl1pt[L1IDX(PA_MBAd(i))] =
			PHYSTOPTE((int)pte & PG_PFNUM) | PG_V|PG_R|PG_M | PG_KW;

		/*
		 * Map the particular MBAd address space into level-2's.
		 */

		for (paddr = PA_MBAd(i); paddr < PA_MBAd(i+1); paddr += NBPG)
			*(int*)(pte++) = PHYSTOPTE(paddr)|PG_V|PG_R|PG_M|PG_KW;
#endif	i386

		/*
		 * If required, map c-list into MBAd mapping registers.
		 * probe_mbad_devices() set mb_clist non-zero if any device
		 * on the mbad needs the c-list.  We panic if can't map
		 * the c-list since we've already 'boot'ed the devices
		 * using it.
		 */

		mb = &MBAd[i];
		if (mb->mb_clist != 0) {
			clist_map = MBAd_mapall(mb, nclistmap);
			if (clist_map < 0) {
				printf("MBAd%d: can't map c-list (%d maps needed, %d available).\n",
					i, nclistmap, MB_MAPS-mb->mb_dmareg);
				panic("c-list map");
				/*
				 *+ An insufficent number of multibus mapping
				 *+ registers exist.
				 *+ Corrective action:
				 *+ either configure this device with more 
				 *+ segments or a larger segsize, or delete 
			         *+ some device(s) from this mbad, as it can
				 *+ provide only a total of 256 DMA maps 
				 *+ spread among all attached devices.
				 */
			}
			mb->mb_clist = mbad_physmap(mb, clist_map, (caddr_t)cfree,
						(u_int)nclist*sizeof(struct cblock),
						nclistmap);
			CPRINTF("c-list @ MBAd%d: %d maps @ %d.\n",
					i, nclistmap, clist_map);
		}

		if (mb->mb_dmareg != MB_MAPS)
			CPRINTF("MBAd%d: %d maps unused.\n",
					i, MB_MAPS-mb->mb_dmareg);
	}
}

/*
 * MBAd_error()
 *	Error interrupts from MBAd's wind up here.
 *
 * Print out logical MBAd number and slic address.
 */

static
MBAd_error(vector)
	u_char	vector;
{
	register int board;

	board = (vector - MBAd_errbase) / MBAD_ERR_ALIGN;
	printf("MBAd%d hard (access) error, slic %d.\n",
			board, MBAd[board].mb_slicaddr);
        /*
         *+ The specified mbad generated an error interrupt.
         */
	access_error((u_char)rdslave(MBAd[board].mb_slicaddr, SL_G_ACCERR));
	panic("MBAd hard error");
        /*
         *+ There was an unrecoverable error on the MULTIBUS Adapter.
         */
}

/*
 * MBAd_mapall()
 *	Allocate map-registers on a MBAd.
 *
 * This is static allocation; once allocated, never released.
 */

static
MBAd_mapall(mb, nmaps)
	register struct mb_desc *mb;
	register int	nmaps;
{
	int	val;

	if (nmaps == 0)
		return (0);

	if (mb->mb_dmareg + nmaps > MB_MAPS)
		return (-1);

	val = mb->mb_dmareg;
	mb->mb_dmareg += nmaps;

	return (val);
}

/*
 * MBAd_probe()
 *	Probe for a device on a multibus.
 *
 * Handles wild-card MBAd specification, finding a MBAd with the device
 * if it exists.
 *
 * Returns pointer to MBAd descriptor that device lives on or zero if
 * device not found.
 */

static struct mb_desc *
MBAd_probe(mbd, md)
	struct mbad_driver *mbd;
	register struct mbad_dev *md;
{
	register int bits;
	register struct mb_desc *mb;
	struct	mbad_probe mp;
	extern	int (*probe_nmi)();

	/*
	 * MBAd is either wild-carded or must exist in the configuration.
	 */

	if (md->md_idx >= 0 && !MBAD_EXISTS(md->md_idx))
		return ((struct mb_desc *)0);

	/*
	 * If no probe procedure, then assume it exists if the
	 * desired MBAd exists.  Note:  MBAD_EXISTS is false for
	 * wildcard MBAd index.
	 */

	if ((mbd->mbd_cflags & MBD_HASPROBE) == 0) {
		if (MBAD_EXISTS(md->md_idx))
			return (&MBAd[md->md_idx]);
		else
			return ((struct mb_desc *)0);
	}

	/*
	 * It has a probe procedure, so we have to try to find it.
	 * We take over NMI vector temporarily here, to handle not
	 * present devices (see machine/locore.s for NMI handling).
	 */
	probe_nmi = MBAd_nmi;

	for (MBAd_idx = 0, bits = MBAdvec; bits != 0; bits >>= 1, ++MBAd_idx) {
		if (!MBAD_EXISTS(MBAd_idx))
			continue;
		if (!WILDCARD(md) && MBAd_idx != md->md_idx)
			continue;

		/*
		 * Either wildcard or specific MBAd and we're at it.
		 * Set up parameters for probe procedure and call
		 * driver.  If driver returns "true", then we found it.
		 *
		 * Use setjmp/longjmp to handle case of device not
		 * present, which results in NMI.
		 */

		mb = &MBAd[MBAd_idx];
		mp.mp_desc = mb;
		mp.mp_csr = md->md_csr;
		mp.mp_flags = md->md_flags;

		if (setjmp(&MBAd_nmi_jmp))
			continue;
		
		if ((*mbd->mbd_probe)(&mp)) {
			probe_nmi = NULL;
			return (mb);
		}
	}

	/*
	 * Ran thru the existing MBAd's and didn't find it;
	 * restore entry NMI vector and return failure.
	 */
	probe_nmi = NULL;
	return ((struct mb_desc *)0);
}

/*
 * MBAd_nmi()
 *	NMI handler while probing MBAd devices.
 *
 * We clear the access error from the processor and longjmp (returning
 * to MBAd_probe, above).
 *
 * This is enterred directly from HW vector table; however, since
 * entry is from drivers 'probe' procedure, MBAd_probe() already
 * considers scratch registers as volitile (thus, no need to save/restore
 * them).
 */

static
MBAd_nmi()
{
#if	defined(ns32000) || defined(KXX)
	u_char	regval =  rdslave(va_slic->sl_procid, SL_G_ACCERR);
	int     timeout;
	/*
	 * Need a "clearnmi" for 32K.
	 */
	timeout = ((~regval)&SLB_ATMSK) == SLB_AETIMOUT;

	/*
	 * Writing anything to the processor's access-error register
	 * clears it.
	 */

	wrslave(va_slic->sl_procid, SL_G_ACCERR, 0xbb);

#else	/*SGS HW*/
	/*
	 * SGS and SGS2 HW.
	 */
	int  timeout;   
	timeout = clearnmi();
#endif	KXX

	/*
	 * Determine why we got an NMI.  Hopefully it was NOT
	 * due to a Sequent Bus Timeout.
	 */

	if (timeout) {
		/*
		 * We COULD deconfigure the board here, and keep on running.
		 * But, we panic instead, just to play it safe, and protect
		 * naive system administrators.
		 */
		printf("mbad %d is NOT FUNCTIONING, SQT BUS TIMEOUT\n", MBAd_idx);
                /*
                 *+ The specified mbad experienced a Sequent bus timeout.
                 */

		panic("mbad TIMEOUT");
                /*
		 *+ The system bus timed out on a request to the 
		 *+ MULTIBUS Adapter.
                 */
	}

	/*
	 * "return" to MBAd_probe.
	 */
	longjmp(&MBAd_nmi_jmp);
}

/*
 * mbad_physmap()
 *	Set up DMA transfer to physical memory in MBAd map registers.
 *
 * addr argument is physical-addr of target/source of DMA.
 * Used during boot/probe in drivers to allow multibus devices to
 * see into system memory.
 *
 * Returns DMA address to tell the multi-bus device.
 *
 * No checking on legality of arguments.
 */

unsigned
mbad_physmap(mb, dmabase, addr, len, nmap)
	register struct	mb_desc	*mb;		/* MBAd descriptor */
	register int	dmabase;		/* 1st map register */
	caddr_t		addr;			/* start address to map */
	unsigned	len;			/* # bytes to map */
	int		nmap;			/* # map registers (sanity) */
{
	register int count;
	unsigned offset;
	unsigned val;

	offset = (int)addr & (MB_MRSIZE-1);
	count = (offset + len + (MB_MRSIZE-1)) / MB_MRSIZE;
	ASSERT(count <= nmap, "mbad_physmap: too big");
        /*
         *+ The number of MULTIBUS Adapter map registers required to
         *+ perform this DMA request was not sufficient for this transfer
         *+ size.
         */

	val = mb->mb_dmabase + dmabase * MB_MRSIZE + offset;

	for (; count--; addr += MB_MRSIZE, ++dmabase) {
		ASSERT(dmabase < MB_MAPS, "mbad_physmap: bad map");
                /*
                 *+ When setting up the MULTIBUS Adapter map registers for
                 *+ a DMA transfer, the kernel found that it was using 
		 *+ nonexistent registers.
                 */
#ifdef	DEBUG
		if (mbad_debug)
			printf("MBAd map[%d] = 0x%x\n", dmabase, PHYSTOMB(addr));
#endif	DEBUG
		ASSERT((u_long) addr < MAX_MBAD_ADDR_MEM, "mbad_physmap: address > 64Meg");
                /*
                 *+ The kernel attempted to map addresses greater than 64M.
                 *+ The MULTIBUS Adapter cannot access physical memory
                 *+ at addresses greater than 64M.
                 */

		MBADMAP(mb, dmabase) = PHYSTOMB(addr);
	}

	return (val);
}

#if	NBPG >= MB_MRSIZE		/* eg, SGS */

/*
 * mbad_setup()
 *	Takes a "bp" (struct buf *), a mb_desc structure,
 *	a DMA register base number, and sets up the DMA.
 *
 * Returns DMA address to tell the multi-bus device.
 * Does *no* locking.
 * Panics if bad pte found; "can't" happen.
 *
 * B_RAWIO, B_PTEIO, B_PTBIO cases must flush TLB to avoid stale mappings
 * thru Usrptmap[], since this is callable from interrupt procedures.
 *
 * Implementation assumes NBPG >= MB_MRSIZE.
 */

unsigned
mbad_setup(bp, mb, dmabase, nmap)
	struct	buf	*bp;			/* buffer header */
	struct	mb_desc	*mb;			/* MBAd descriptor */
	register int	dmabase;		/* 1st map register */
	int	nmap;				/* # map registers (sanity) */
{
	register struct pte *pte;		/* pte mapping data */
	register unsigned paddr;		/* phys-addr to map */
	int	count;				/* # MB maps to set up */
	unsigned val;				/* MB dma address to use */
	unsigned pgoffset;			/* RAW IO offset in page */
	unsigned mboffset;			/* RAW IO offset in MB map */
	struct pte *vtopte();

	/*
	 * Source/target pte's are found differently based on type
	 * of IO operation.
	 */

	switch(bp->b_iotype) {
	case B_RAWIO:					/* RAW IO */
		/*
		 * In this case, must look into alignment of physical
		 * memory, since we can start on arbitrary boundary.
		 */

		flush_tlb();
		pte = vtopte(bp->b_proc, btop(bp->b_un.b_addr));
		pgoffset = (int)bp->b_un.b_addr & (NBPG-1);
		mboffset = pgoffset & (MB_MRSIZE-1);
		count = (unsigned)(mboffset + bp->b_bcount + (MB_MRSIZE-1))
						>> MB_MRSIZEL2;
		val = mb->mb_dmabase + dmabase * MB_MRSIZE + mboffset;
		paddr = PTETOPHYS(*pte) + (pgoffset & ~(MB_MRSIZE-1));
		break;

	case B_FILIO:					/* file-sys IO */
		/*
		 * Filesys/buffer-cache IO.  These are always cluster aligned
		 * both physically and virtually.
		 */

		pte = &Sysmap[btop(bp->b_un.b_addr)];
		count = (unsigned)(bp->b_bcount + MB_MRSIZE-1) >> MB_MRSIZEL2;
		val = mb->mb_dmabase + dmabase * MB_MRSIZE;
		paddr = PTETOPHYS(*pte);
#ifdef	DEBUG
		if (((int)bp->b_un.b_addr & (MB_MRSIZE-1)) != 0) {
			printf("bp=0x%x, addr=0x%x\n", bp, bp->b_un.b_addr);
			panic("mbad_setup: bad FS IO");
		}

		/*
		 * Kludge for MULTIBUS tape driver on systems with
		 * > 64 Meg of memory.  B_PHYS is used to know this
		 * is a tape request.
		 */

		if ((bp->b_flags & B_PHYS) == 0
		&&  (bp->b_bcount <= 0 || bp->b_bcount > MAXBSIZE)) {
			printf("bp=0x%x, bcount=%d\n", bp, bp->b_bcount);
			panic("mbad_setup: bad FS count");
		}
#endif	DEBUG
		break;


	case B_PTBIO:					/* Page-Table IO */
		/*
		 * Page-Table IO: like B_PTEIO, but can start/end with
		 * non-cluster aligned memory (but is always HW page
		 * aligned).  Count is multiple of NBPG.
		 *
		 * Separate case for greater efficiency in B_PTEIO.
		 *
		 * On SGS, this is identical to B_PTEIO, so fall into...
		 */

	case B_PTEIO:					/* swap/page IO */
		/*
		 * Pte-based IO -- already know pte of 1st page, which
		 * is cluster aligned, and b_count is a multiple of CLBYTES.
		 */

		flush_tlb();
		pte = bp->b_un.b_pte;
		count = (unsigned)(bp->b_bcount) >> MB_MRSIZEL2;
		val = mb->mb_dmabase + dmabase * MB_MRSIZE;
		paddr = PTETOPHYS(*pte);
		break;

	default:
		panic("mbad_setup: bad b_iotype");
                /*
                 *+ The setup function to program the MULTIBUS Adapter map
                 *+ registers for a DMA transfer was called with an invalid
                 *+ buffer type describing the I/O request.
                 */
		/*NOTREACHED*/
	}

	/*
	 * Check count and set up the map registers.
	 * Careful to pick up new phys-address when cross page boundary.
	 */

	ASSERT(count <= nmap, "mbad_setup: too big");
        /*
         *+ The number of MULTIBUS Adapter map registers required to
         *+ perform this DMA request was not sufficient for this transfer
         *+ size.
         */
	ASSERT(dmabase + count <= MB_MAPS, "mbad_setup: bad map");
        /*
         *+ When setting up the MULTIBUS Adapter map registers for
         *+ a DMA transfer, the kernel found it was using nonexistent
         *+ registers.
         */

	while (count--) {
#ifdef	DEBUG
		if (mbad_debug)
			printf("MBAd map[%d] = 0x%x\n",dmabase,PHYSTOMB(paddr));
#endif	DEBUG
		ASSERT(paddr<MAX_MBAD_ADDR_MEM, "mbad_setup: address >= 64Meg");
                /*
                 *+ The kernel attempted to map addresses greater than 64M.
                 *+ The MULTIBUS Adapter cannot access physical memory
                 *+ at addresses greater than 64M.
                 */
		MBADMAP(mb, dmabase) = PHYSTOMB(paddr);
		++dmabase;
		paddr += MB_MRSIZE;
		if ((paddr & (NBPG-1)) == 0 && count) {
			++pte;
			ASSERT_DEBUG(PTEPF(*pte)!=0, "mbad_setup: null pfnum");
			paddr = PTETOPHYS(*pte);
		}
	}

	return (val);
}

#else	NBPG < MB_MRSIZE			/* eg, FGS */

/*
 * mbad_setup()
 *	Takes a "bp" (struct buf *), a mb_desc structure,
 *	a DMA register base number, and sets up the DMA.
 *
 * Returns DMA address to tell the multi-bus device.
 * Does *no* locking.
 * Panics if bad pte found; "can't" happen.
 *
 * Implementation assumes NBPG <= MB_MRSIZE <= CLBYTES.
 *
 * Don't bother checking MBAD physical addressing (vs 64Meg), since
 * all current small page-size systems are limited to < 64Meg already.
 */

unsigned
mbad_setup(bp, mb, dmabase, nmap)
	register struct	buf	*bp;		/* buffer header */
	register struct	mb_desc	*mb;		/* MBAd descriptor */
	register int	dmabase;		/* 1st map register */
	int	nmap;				/* # map registers (sanity) */
{
	register struct pte *pte;
	register int count;
	unsigned offset;
	unsigned val;
	struct pte *vtopte();

	/*
	 * Source/target pte's are found differently based on type
	 * of IO operation.
	 */

	switch(bp->b_iotype) {
	case B_RAWIO:					/* RAW IO */
		/*
		 * In this case, must look into alignment of physical
		 * memory, since we can start on arbitrary boundary.
		 */

		flush_tlb();
		pte = vtopte(bp->b_proc, btop(bp->b_un.b_addr));
		offset = PTEMBOFF(*pte) + ((int)bp->b_un.b_addr & (NBPG-1));
		pte -= btop(offset);
		count = (offset + bp->b_bcount + (MB_MRSIZE-1)) / MB_MRSIZE;
		break;

	case B_FILIO:					/* file-sys IO */
		/*
		 * Filesys/buffer-cache IO.  These are always cluster aligned
		 * both physically and virtually.
		 */

		pte = &Sysmap[btop(bp->b_un.b_addr)];
		count = (bp->b_bcount + MB_MRSIZE-1) / MB_MRSIZE;
		offset = 0;
#ifdef	DEBUG
		if (((int)bp->b_un.b_addr & (MB_MRSIZE-1)) != 0) {
			printf("bp=0x%x, addr=0x%x\n", bp, bp->b_un.b_addr);
			panic("mbad_setup: bad FS IO");
		}
		if (bp->b_bcount <= 0 || bp->b_bcount > MAXBSIZE) {
			printf("bp=0x%x, bcount=%d\n", bp, bp->b_bcount);
			panic("mbad_setup: bad FS count");
		}
#endif	DEBUG
		break;

	case B_PTEIO:					/* swap/page IO */
		/*
		 * Pte-based IO -- already know pte of 1st page, which
		 * is cluster aligned, and b_count is a multiple of CLBYTES.
		 */

		flush_tlb();
		pte = bp->b_un.b_pte;
		count = (bp->b_bcount + MB_MRSIZE-1) / MB_MRSIZE;
		offset = 0;
		break;

	case B_PTBIO:					/* Page-Table IO */
		/*
		 * Page-Table IO: like B_PTEIO, but can start/end with
		 * non-cluster aligned memory (but is always HW page
		 * aligned).  Count is multiple of NBPG.
		 *
		 * Separate case for greater efficiency in B_PTEIO.
		 */

		flush_tlb();
		pte = bp->b_un.b_pte;
		offset = PTEMBOFF(*pte);
		pte -= btop(offset);
		count = (offset + bp->b_bcount + (MB_MRSIZE-1)) / MB_MRSIZE;
		break;

	default:
		panic("mbad_setup: bad b_iotype");
                /*
                 *+ The setup function to program the MULTIBUS Adapter map
                 *+ registers for a DMA transfer was called with an invalid
                 *+ buffer type describing the I/O request.
                 */
		/*NOTREACHED*/
	}

	/*
	 * Check count, figure return value, and set up the map registers.
	 * This code is a clone of mbad_dma().
	 */

	ASSERT(count <= nmap, "mbad_setup: too big");
        /*
         *+ The number of MULTIBUS Adapter map registers required to
         *+ perform this DMA request was not sufficient for this transfer
         *+ size.
         */

	val = mb->mb_dmabase + dmabase * MB_MRSIZE + offset;

	for(; count--; pte += (MB_MRSIZE/NBPG), ++dmabase) {
		ASSERT(dmabase < MB_MAPS, "mbad_setup: bad map");
		/*
		 *+ When setting up the MULTIBUS Adapter map registers for
		 *+ a DMA transfer, the kernel found it was using nonexistent
		 *+ registers.
		 */

#ifdef	DEBUG
		ASSERT(PTEPF(*pte) != 0, "mbad_setup: null pfnum");
		ASSERT((PTETOPHYS(*pte) & (MB_MRSIZE-1)) == 0, "mbad_setup: unaligned");
		if (mbad_debug)
			printf("MBAd map[%d] = 0x%x\n", dmabase, PHYSTOMB(PTETOPHYS(*pte)));
#endif	DEBUG
		MBADMAP(mb, dmabase) = PHYSTOMB(PTETOPHYS(*pte));
	}

	return (val);
}

#endif	NBPG >= MB_MRSIZE

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
static	char	rcsid[] = "$Header: vlsi.c 1.1 89/08/24 $";
#endif

/*
 * vlsi.c
 *	VLSI performance driver.
 *
 * Semantics:
 *	+ This is a pseudo-device (eg, software driver).
 *	+ ioctl's to manipulate counters.
 *	+ read (no write) to access performance data.
 */

/* $Log:	vlsi.c,v $
 */

#include "../h/types.h"
#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/errno.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../balance/cfg.h"
#include "../balance/engine.h"
#include "../balance/slic.h"
#include "../balance/SGSproc.h"
#include "../balance/bic.h"
#include "../balance/cmc.h"
#include "../machine/intctl.h"
#include "../machine/vlsi.h"

/*
 * Driver local variables.
 */

static struct vlsiinfo	vlsi;

/*
 * Data structures to allow parameterized access to the various VLSI
 * counter registers (select, mask, and the counters themselves)
 */

static u_char cmc_cntrs[VLSI_CMC_CNTRS][4] = {
	CMC_P_CNTR1_B0, CMC_P_CNTR1_B1, CMC_P_CNTR1_B2, CMC_P_CNTR1_B3,
	CMC_P_CNTR2_B0, CMC_P_CNTR2_B1, CMC_P_CNTR2_B2, CMC_P_CNTR2_B3
};

static u_char cmc_select[VLSI_CMC_CNTRS] = { CMC_P_SELECT1, CMC_P_SELECT2 };

static u_char cmc_mask[VLSI_CMC_CNTRS][3] = {
	CMC_P_MASK1_B0, CMC_P_MASK1_B1, CMC_P_MASK1_B2,
	CMC_P_MASK2_B0, CMC_P_MASK2_B1, CMC_P_MASK2_B2
};

static u_char procbic_cntr[] = {
	BIC_TACH0, BIC_TACH1, BIC_TACH2, BIC_TACH3
};

extern struct ctlr_desc *slic_to_config[];

/*
 * Forward references for static functions
 */
extern void start_vlsi_cntrs(), stop_vlsi_cntrs();
extern int setup_procbic_cntr(), setup_cmc_cntr();
extern void set_cmc_sel_mask();
extern void set_bic_sel_mask();

/*
 * vlsiboot()
 *	Allocate space to hold data read from VLSI.
 */

vlsiboot()
{
	register struct ctlr_desc *cd;
	register int i;
	u_char *bsp;
	u_char slicid, rev;
	u_long bics_counted;	/* bit vector for marking BICs accounted for */

	init_sema(&vlsi.vi_mutex, 1, 0, vlsi_gate);

	/*
	 * First count the number of CMCs and BICs, check CMC revs.
	 */
	bics_counted = 0;
	vlsi.vi_proc_ncmc = (u_char *) calloc((int)(Nengine * sizeof(u_char)));
	vlsi.vi_cmcrev = CMCV_REV_MASK;	/* init to highest possible rev */

	for (i = 0; i < Nengine; i++) {
		slicid = engine[i].e_slicaddr;
		cd = slic_to_config[slicid];
		vlsi.vi_ncmc += cd->cd_p_nsets;
		vlsi.vi_proc_ncmc[i] = cd->cd_p_nsets;
		/*
		 * Read the CMC revision and update vi_cmcrev.
		 * If mixed rev's, keep the lowest rev as the
		 * global rev.
		 */
		rev = rdSubslave(slicid, PROC_CMC_0, CMC_VERSION);
		rev = CMCV_REVISION(rev);
		if (rev < vlsi.vi_cmcrev)
			vlsi.vi_cmcrev = rev;

		slicid = BIC_SLIC(slicid, cd->cd_flags);
		if ((bics_counted & (1 << slicid/2)) == 0) {
			bics_counted |= (1 << slicid/2);
			vlsi.vi_nprocbic++;
		}
	}

	vlsi.vi_procbic_slic = (u_char *)
			calloc((int)(vlsi.vi_nprocbic * sizeof(u_char)));
	bics_counted = 0;
	bsp = vlsi.vi_procbic_slic;

	for (i = 0; i < Nengine; i++) {
		slicid = engine[i].e_slicaddr;
		cd = slic_to_config[slicid];
		slicid = BIC_SLIC(slicid, cd->cd_flags);
		if ((bics_counted & (1 << slicid/2)) == 0) {
			bics_counted |= (1 << slicid/2);
			*bsp++ = slicid;
		}
	}

	/*
	 * Allocate data buffer
	 */
	vlsi.vi_data = (u_int *) calloc((int)((vlsi.vi_ncmc * VLSI_CMC_CNTRS +
		vlsi.vi_nprocbic * VLSI_BIC_CNTRS) * sizeof(unsigned)));
}

/*
 * vlsiopen()
 *	"Open" the vlsi device -- enforces exclusive access.
 */

vlsiopen(dev)
	dev_t	dev;
{
	int	error = 0;

	/*
	 * Only one of these
	 */
	if (minor(dev) > 0)
		return(ENXIO);

	/*
	 * Does this need to be a sema?  Lock would do.
	 */
	p_sema(&vlsi.vi_mutex, PZERO-1);

	if (vlsi.vi_state & VLSI_INUSE)
		error = EBUSY;
	else
		vlsi.vi_state |= VLSI_INUSE;

	v_sema(&vlsi.vi_mutex);

	return(error);
}

/*
 * vlsiclose()
 *	Close (release) the VLSI performance instrumentation.
 */

/*ARGSUSED*/
vlsiclose(dev)
	dev_t	dev;
{
	/*
	 * If counters still running, stop them first
	 */
	if (vlsi.vi_state & VLSI_STARTED) {
		stop_vlsi_cntrs();
	}
	p_sema(&vlsi.vi_mutex, PZERO-1);
	vlsi.vi_state &= ~VLSI_INUSE;
	v_sema(&vlsi.vi_mutex);
}

/*
 * vlsiread()
 *	Read data from the VLSI counters.
 */
/*ARGSUSED*/
vlsiread(dev, uio)
	dev_t	dev;
	struct uio *uio;
{
	register u_int *dp;
	register int i;
	int len;
	extern u_int read_cntr();	/* forward ref */

	/*
	 * Stop the VLSI counters, read them all,  and restart them.
	 */
	stop_vlsi_cntrs();
	dp = vlsi.vi_data;
	for (i = 0; i < Nengine; i++) {
		register u_char slicid = engine[i].e_slicaddr;

		*dp++ = read_cntr(slicid, PROC_CMC_0, cmc_cntrs[0]);
		*dp++ = read_cntr(slicid, PROC_CMC_0, cmc_cntrs[1]);
		if (vlsi.vi_proc_ncmc[i] == 2) {
			*dp++ = read_cntr(slicid, PROC_CMC_1, cmc_cntrs[0]);
			*dp++ = read_cntr(slicid, PROC_CMC_1, cmc_cntrs[1]);
		}
	}
	/*
	 * Read all BICS that are available (reachable by at least one
	 * good processor)
 	 */
	{
	    register u_char *bsp = vlsi.vi_procbic_slic;

	    for (i = 0; i < vlsi.vi_nprocbic; i++) {
		*dp++ = read_cntr(*bsp, PROC_BIC, procbic_cntr);
		bsp++;
	    }
	}
	start_vlsi_cntrs();
	len = MIN((dp - vlsi.vi_data) * sizeof(u_int), uio->uio_resid);
	return(uiomove((caddr_t)vlsi.vi_data, len, UIO_READ, uio));
}

/*
 * Read a VLSI performance counter
 */

static u_int
read_cntr(slicid, reg, subregs)
	u_char slicid;
	u_char reg;
	u_char *subregs;
{
	register int k;
	register u_int val = 0;

	/*
	 * Slightly slower than copying bytes, but portable
	 */
	for (k = 0; k < 32; k += 8, subregs++) {
		val |= (u_int) rdSubslave(slicid, reg, *subregs) << k;
	}
	return(val);
}

/*
 * vlsiioctl()
 *	ioctl's to manipulate VLSI mask/select regs and to start/stop
 *	data collection.
 */

/*ARGSUSED*/
vlsiioctl(dev, com, data)
	dev_t		dev;
	int		com;
	caddr_t		data;
{
	int	error = 0;
	struct vlsi_cfg vcfg;

	switch(com) {

	case VLSIIOCSTART:
		start_vlsi_cntrs();
		break;

	case VLSIIOCSTOP:
		stop_vlsi_cntrs();
		break;

	case VLSIIOCGETCFG:
		vcfg.vc_nprocbic = vlsi.vi_nprocbic;
		vcfg.vc_ncmc = vlsi.vi_ncmc;
		vcfg.vc_cmcrev = vlsi.vi_cmcrev;
		bcopy((caddr_t)&vcfg, data, sizeof(vcfg));
		break;

	case VLSIIOCGETCMCP:
		error = copyout((char *) vlsi.vi_proc_ncmc,
				(char *) *((char **)data),
				(u_int) Nengine*sizeof(u_char));
		break;

	case VLSIIOCSETCMC:
		error = setup_cmc_cntr((struct cmc_cntr_setup *)data);
		break;

	case VLSIIOCSETPROCBIC:
		error = setup_procbic_cntr((struct bic_cntr_setup *)data);
		break;

	default:
		error = ENXIO;
		break;
	}

	return(error);
}


/*
 * (Clear and) start the VLSI counters counting
 */
static void
start_vlsi_cntrs()
{
#ifdef	WHEN_DISABLE_BOTH_CLEARS
	wrSubslave(SL_GROUP | TMPOS_GROUP, PROC_BIC, BIC_TACH_ZSTART, 0);
	wrSubslave(SL_GROUP | TMPOS_GROUP, PROC_CMC_0, CMC_P_DISABLE_BOTH, 0);
	wrSubslave(SL_GROUP | TMPOS_GROUP, PROC_CMC_1, CMC_P_DISABLE_BOTH, 0);
#else	/* ENABLE_AND_CLEAR_EACH_ONE */
	wrSubslave(SL_GROUP | TMPOS_GROUP, PROC_BIC, BIC_TACH_ZSTART, 0);
	wrSubslave(SL_GROUP | TMPOS_GROUP, PROC_CMC_0, CMC_P_DISABLE1, 0);
	wrSubslave(SL_GROUP | TMPOS_GROUP, PROC_CMC_0, CMC_P_DISABLE2, 0);
	wrSubslave(SL_GROUP | TMPOS_GROUP, PROC_CMC_1, CMC_P_DISABLE1, 0);
	wrSubslave(SL_GROUP | TMPOS_GROUP, PROC_CMC_1, CMC_P_DISABLE2, 0);
#endif	/* WHEN_DISABLE_BOTH_CLEARS */
	vlsi.vi_state |= VLSI_STARTED;
}

/*
 * Stop the VLSI counters
 */
static void
stop_vlsi_cntrs()
{
	(void) rdSubslave(SL_GROUP | TMPOS_GROUP, PROC_BIC, BIC_TACH_ZSTART);
	(void) rdSubslave(SL_GROUP | TMPOS_GROUP, PROC_CMC_0,
							CMC_P_DISABLE_BOTH);
	(void) rdSubslave(SL_GROUP | TMPOS_GROUP, PROC_CMC_1,
							CMC_P_DISABLE_BOTH);
	vlsi.vi_state &= ~VLSI_STARTED;
}


/*
 * Set up CMC select and mask registers for all processors
 */

static int
setup_cmc_cntr(cmc_setup)
	register struct cmc_cntr_setup *cmc_setup;
{
	register u_char slicid;
	register int i;
	struct ctlr_desc *cd;

	/*
	 * Specifying legit cntr? (two counters, number them 0 and 1)
	 */
	if (cmc_setup->cs_cntr > 1)
		return(EINVAL);

	/*
	 * Set up the CMC counter in all CMCs on all processors
	 */
	for (i = 0; i < Nengine; i++) {
		slicid = engine[i].e_slicaddr;
		cd = slic_to_config[slicid];
		/*
		 * Always at least 1 set (CMC).  Simpler/cheaper to check for
		 * 2nd set than to loop.
		 */
		set_cmc_sel_mask(slicid, PROC_CMC_0, cmc_setup);
		if (cd->cd_p_nsets == 2) {
			set_cmc_sel_mask(slicid, PROC_CMC_1, cmc_setup);
		}
	}
	return(0);
}

static void
set_cmc_sel_mask(slicid, proc_cmc, cmc_setup)
	u_char slicid;
	u_char proc_cmc;
	struct cmc_cntr_setup *cmc_setup;
{
	register int i;
	register u_long mask = cmc_setup->cs_mask;
	register u_int cntr = cmc_setup->cs_cntr;

	/*
	 * Initialize CMC perf counter
	 */
	wrSubslave(slicid, proc_cmc, cmc_select[cntr], cmc_setup->cs_select);
	for (i = 0; i < 3; i++) {
		wrSubslave(slicid, proc_cmc, cmc_mask[cntr][i],
			(u_char) (mask & 0xff));
		mask >>= 8;
	}
}

/*
 * Set the select and mask registers for a processor BIC
 */
static int
setup_procbic_cntr(bic_setup)
	struct bic_cntr_setup *bic_setup;
{
	u_char slicid;

	if (bic_setup->bs_bicnum > vlsi.vi_nprocbic)
		return(EINVAL);

	slicid = vlsi.vi_procbic_slic[bic_setup->bs_bicnum];
	set_bic_sel_mask(slicid, PROC_BIC, bic_setup);
	return(0);
}

static void
set_bic_sel_mask(slicid, bic, bic_setup)
	u_char slicid;
	u_char bic;
	struct bic_cntr_setup *bic_setup;
{
	wrSubslave(slicid, bic, BIC_TACH_SEL, bic_setup->bs_select0);
	wrSubslave(slicid, bic, BIC_TACH_MASK, bic_setup->bs_mask0);
	wrSubslave(slicid, bic, BIC_TACH_SEL1, bic_setup->bs_select1);
	wrSubslave(slicid, bic, BIC_TACH_MASK1, bic_setup->bs_mask1);
}

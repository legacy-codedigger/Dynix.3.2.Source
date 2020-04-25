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
static	char	rcsid[] = "$Header: memory.c 2.20 1991/04/30 23:54:13 $";
#endif

/*
 * memory.c
 *	Architecture dependent memory handling routines to deal with
 *	configration, initialization, and error polling.
 *
 * Balance/Symmetry version.
 */

/* $Log: memory.c,v $
 *
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/vfs.h"
#include "../h/vnode.h"
#include "../h/cmn_err.h"
#include "../ufs/mount.h"

#include "../balance/cfg.h"
#include "../balance/engine.h"
#include "../balance/clkarb.h"
#include "../balance/clock.h"
#include "../balance/slic.h"
#include "../balance/slicreg.h"
#include "../balance/bdp.h"
#include "../balance/SGSmem.h"

#include "../machine/pte.h"
#include "../machine/intctl.h"
#include "../machine/plocal.h"
#include "../machine/hwparam.h"
#ifdef	ns32000
#include "../machine/gate.h"
#endif	ns32000

caddr_t		topmem;				/* top of memory */
u_int		totalmem;			/* total memory (topmem-holes)*/
u_int		mc_click;			/* memory bitmap click */

lock_t		uncmem_lock;			/* synch multiple ECC errors */
extern	int	memintvl;			/* polling interval in Hz */

/*
 * Define structure to represent the different types of memory boards.
 */

struct	memory	{
	u_char	m_type;			/* board type */
	int	(*m_init)();		/* procedure to init m_type boards */
	int	(*m_poll)();		/* procedure to poll m_type boards */
};

int	fgs_mem_init(), fgs_mem_poll();
int	sgs_mem_init(), sgs_mem_poll();

static	struct	memory	memory_boards[] = {
	{ SLB_MEMBOARD,	fgs_mem_init, fgs_mem_poll },
	{ SLB_SGSMEMBOARD,sgs_mem_init, sgs_mem_poll },
	{ 0 }
};

/*
 * conf_mem()
 *	Configure memory.
 *
 * Figure top of memory.  Allow holes; meminit() handles this.
 */

conf_mem()
{
	register struct ctlr_toc *toc;
	register struct	ctlr_desc *cd;
	register struct memory *mem;
	register int	i;
	int	bad_mem = 0;
	extern	int	resphysmem;

	/*
	 * Print about deconfigured/failed boards.  These are not in
	 * the bit-map (power-up arranges this).
	 */

	for (mem = memory_boards; mem->m_type != 0; mem++) {
		toc = &CD_LOC->c_toc[mem->m_type];
		cd = &CD_LOC->c_ctlrs[toc->ct_start];
		for (i = 0; i < toc->ct_count; i++, cd++) {
			if (cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF)) {
				if (++bad_mem == 1) {
					printf("Not using memory boards: slic");
					/*
					*+ This is an informative
					*+ message listing memory
					*+ boards that are not
					*+ being used because they
					*+ are deconfigured or
					*+ have failed.
					*/
				}
				CPRINTF(" %d", cd->cd_slic);
			}
		}
	}
	if (bad_mem)
		CPRINTF(".\n");

	/*
	 * Determine top of physical memory, including holes.
	 */

	topmem = (caddr_t) (CD_LOC->c_maxmem);
	totalmem = CD_LOC->c_memsize * 1024;
	mc_click = MC_CLICK;

#ifdef	MAX_PROC_ADDR_MEM
	/*
	 * Limit topmem to what processors can address in main-memory.
	 * Relevant for 032 and K20 processors.
	 */
	if (topmem >= (caddr_t)MAX_PROC_ADDR_MEM) {
		cmn_err(CE_NOTE,"Non-addressable memory [0x%x,0x%x] unused.\n",
				MAX_PROC_ADDR_MEM, topmem);
		/*
		 *+ Memory listed by this message is not addresssible
		 *+ by this configuration, and will be unused.
		 */

		/*
		 * Decrememnt total mem.
		 */
		while (topmem > (caddr_t)MAX_PROC_ADDR_MEM) {
			if (page_exists((int)btop(topmem)))
				totalmem -= mc_click;
			topmem -= mc_click;
		}

		/*
		 * Find next real top of memory.
		 */
		while (!page_exists((int)btop(topmem))) {
			topmem -= mc_click;
		}
		topmem += mc_click;
	}
#endif	MAX_PROC_ADDR_MEM

	/*
	 * If desired, reserve top of physical memory for special things.
	 * Basic sanity: insure at least 2Meg of usable system memory.
	 * This allows resphysmem to consume the highest non-hole, leaving
	 * topmem just after a hole.  meminit() handles this.
	 */

	if (resphysmem) {
		i = (int) topmem - resphysmem;
		while (i < (int) topmem) {
			if (MC_MMAP(i/MC_CLICK, CD_LOC) == 0) {
				printf("Reserved memory has a hole; no reserved memory allocated.\n");
                                /*
                                 *+ The physical memory space reserved
                                 *+ by the configuration variable
                                 *+ resphysmem contains a hole.  The
                                 *+ existence of this hole has caused
                                 *+ the kernel to ignore the request to
                                 *+ reserve physical memory.
                                 */
				resphysmem = 0;
				break;
			}
			i += MC_CLICK;
		}
		if ((int) topmem - resphysmem < 2*1024*1024) {
			printf("Not enough memory to reserve 0x%x bytes.\n",
							resphysmem);
                        /*
                         *+ The system doesn't have enough physical memory
                         *+ available to reserve the physical memory 
			 *+ requested by the configuration variable resphysmem.
                         *+ The kernel has ignored the request to
                         *+ reserve physical memory.
                         */

			resphysmem = 0;
		}
		if (resphysmem) {
			topmem -= resphysmem;
			totalmem -= resphysmem;
			printf("Physical memory [0x%x,0x%x) reserved for custom use.\n",
					topmem, topmem+resphysmem);
                        /*
                         *+ The kernel has reserved physical memory
                         *+ as requested by the resphsysmem
                         *+ configuration variable.
                         */
		}
	}
}

/*
 * page_exists()
 *	Return true iff HW page "pg" exists in physical memory.
 */

page_exists(pg)
	register int pg;
{
	pg /= btop(MC_CLICK);
	return((pg >= CD_LOC->c_mmap_size) ? 0 : MC_MMAP(pg, CD_LOC));
}


/*
 * memleft()
 *	Return amount of memory left from given point to top of memory
 */
u_int
memleft(mem)
	u_int mem;
{
	register int x;
	int pg, top, clicks = 0;
	u_int result;

	pg = (mem / mc_click)+1;
	top = (u_int)topmem / mc_click;
	for (x = pg; x < top; ++x)
		if (MC_MMAP(x, CD_LOC))
			++clicks;
	result = (clicks * mc_click) + (mc_click - (mem % mc_click));
	return(result);
}

/*
 * memenable()
 *	Enable memory controller correctable and uncorrectable
 *	error reporting.
 *
 * The ecc correctable errors are polled for at memintvl seconds
 * (usually 10 minutes).  Thus we report at most once per memintvl.
 */

memenable()
{
	register struct ctlr_toc *toc;
	register struct	ctlr_desc *cd;
	register struct memory *mem;
	register int i;

	/*
	 * uncmem_lock is used to synchronize if multiple ecc uncorrectable
	 * errors occur concurrently.
	 */

	init_lock(&uncmem_lock, G_ENGINE);

	/*
	 * Initialize all instances of all kinds of memory boards.
	 */

	for (mem = memory_boards; mem->m_type != 0; mem++) {
		toc = &CD_LOC->c_toc[mem->m_type];
		cd = &CD_LOC->c_ctlrs[toc->ct_start];
		for (i = 0; i < toc->ct_count; i++, cd++) {
			if ((cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF)) == 0)
				(*mem->m_init)(cd);
		}
	}
}

/*
 * memlog()
 *	Log memory errors.
 *
 * This routine checks for memory errors.  It is called when ecc uncorrectable
 * errors are detected AND when polling for ecc correctable memory errors.
 * Its functions is to log and clear, if possible, existing errors.
 */

memlog()
{
	register struct ctlr_toc *toc;
	register struct	ctlr_desc *cd;
	register struct memory *mem;
	register int i;

	/*
	 * Poll all instances of all kinds of memory boards for errors.
	 */

	for (mem = memory_boards; mem->m_type != 0; mem++) {
		toc = &CD_LOC->c_toc[mem->m_type];
		cd = &CD_LOC->c_ctlrs[toc->ct_start];
		for (i = 0; i < toc->ct_count; i++, cd++) {
			if ((cd->cd_diag_flag & (CFG_FAIL|CFG_DECONF)) == 0)
				(*mem->m_poll)(cd);
		}
	}
}

/*
 * memerr()
 *	Memerr is the timeout routine which polls for ecc corrected memory
 *	errors.
 */

memerr()
{
	memlog();
	timeout(memerr, (caddr_t)0, memintvl);
}

/*
 * fgs_mem_init()
 *	Arrange error handling for a given FGS memory board.
 */

static
fgs_mem_init(cd)
	register struct	ctlr_desc *cd;
{
	spl_t s_spl;
	/*
	 * Enable Ecc correctable and uncorrectable error logging and
	 * uncorrectable error reporting.
	 */
	 s_spl = splhi();
	wrslave(cd->cd_slic, SL_M_ECC, SLB_EN_UCE_LOG|SLB_REP_UCE|SLB_EN_CE_LOG);
	splx(s_spl);
}

/*
 * fgs_mem_poll()
 *	Poll a given FGS memory board for errors.
 *
 * Log and clear, if possible existing errors.
 */

static
fgs_mem_poll(cd)
	register struct	ctlr_desc *cd;
{
	u_char	eccreg;
	u_char	ilv;
	u_char	val;
	u_char	slbsize;
	u_char	syndrome;
	u_char	row;
	int	addr;
	spl_t	s_ipl;

	/*
	 * Read memory board ECC register.
	 */

	s_ipl = splhi();
	eccreg = rdslave(cd->cd_slic, SL_M_ECC);
	splx(s_ipl);

	/*
	 * If no errors, return.  Else print about it and get more data.
	 */

	if ((eccreg & (SLB_UCE|SLB_UCE_OV|SLB_CE|SLB_CE_OV)) == 0)
		return;

	printf("%s %d: ECC %scorrectable error: Slic id = %d, Error Reg = 0x%x,\n",
			cd->cd_name, cd->cd_i,
			(eccreg & (SLB_UCE|SLB_UCE_OV)) ? "Un" : "",
			cd->cd_slic, eccreg);
	/*
	 *+ A correctible memory error has occurred.
	 *+ No corrective action is required.
	 */

	/*
	 * Get the base address of the board.
	 */

	s_ipl = splhi();
	ilv = rdslave(cd->cd_slic, SL_M_ENABLES);
	val = rdslave(cd->cd_slic, SL_M_ADDR);
	slbsize = rdslave(cd->cd_slic, SL_M_BSIZE);
	splx(s_ipl);

	/*
	 * Determine interleaving.
	 */

	if ((ilv & SLB_INTLV) && (val&1))
		val &= ~1;

	/*
	 * Base address.
	 */

	if (slbsize & SLB_LTYPE)
		addr = val << 19;	/* 64K chips */
	else
		addr = ((val & 0xE0) << 19) | ((val & 0x07) << 21);

	if ((eccreg & (SLB_UCE|SLB_CE)) == 0) {
		/*
		 * Clear the error and report.
		 */
		s_ipl = splhi();
		wrslave(cd->cd_slic, SL_M_ECC, eccreg);
		splx(s_ipl);
		CPRINTF("%s %d: Base Address = 0x%x, Operation = ?, Syndrome = ?\n",
				cd->cd_name, cd->cd_i, addr);
		return;
	}

	/*
	 * Read the ECC error address and determine the row.
	 */

	s_ipl = splhi();
	addr &= 0xff000000;	/* mask out lower 24 bits */
	val = rdslave(cd->cd_slic, SL_M_EADD_H);
	addr += val << 16;
	val = rdslave(cd->cd_slic, SL_M_EADD_M);
	addr += val << 8;
	val = rdslave(cd->cd_slic, SL_M_EADD_L);
	addr += val & ~SLB_ROW;
	row = val & SLB_ROW;

	/*
	 * Get operation code and syndrome.
	 */

	val = rdslave(cd->cd_slic, SL_M_ES);
	syndrome = rdslave(cd->cd_slic, SL_M_SYNDR);

	/*
	 * Clear the error and report.
	 */

	wrslave(cd->cd_slic, SL_M_ECC, eccreg);
	splx(s_ipl);
	CPRINTF("%s %d: Address = 0x%x (%s Board), Operation = 0x%x, Syndrome = 0x%x\n",
		cd->cd_name, cd->cd_i,
		addr, (row == SLB_CNTRL) ? "Controller" : "Expansion",
		val, syndrome);
}

/*
 * sgs_mem_init()
 *	Arrange error handling for a given SGS memory board.
 *
 * Power-up firmware leaves the board enabled and in "if loggable" mode;
 * ie, scrubbing is done if it can log an error it finds.  This is
 * how Dynix wants to run the memory board, so nothing to do.
 */

static
sgs_mem_init(cd)
	register struct	ctlr_desc *cd;
{
#ifdef	lint
	cd->cd_slic = 0;
#endif	lint
}

/*
 * sgs_mem_poll()
 *	Poll a given SGS memory board for errors.
 *
 * Read the EDC Error Register and see if there are any errors.
 * Single bit errors can be located to address and bit; double bit
 * errors can only be isolated to the address.
 *
 * Much of this code is cloned from /ccs/sced.fw/v.next/work/src/fw/mon/mem2.c
 * (SCED FW power-up memory test).
 *
 * Smarter polling/recording/etc should be done (later) that might
 * (eg) turn off scrubbing for boards with a bad bank (don't want to
 * get reports all the time for known bad bits), or be more intelligent
 * about how often and how to report.
 */

/*
 * sgs_mem_decode[]
 *	Table indexed by syndrome value for single-bit error, returns
 *	which bit is in error.
 *
 * The value is described below:
 *
 *	0 - 31 indicates the failing data bit position. 0 is the lsb and
 *	       31 is the msb.
 *
 *	32- 38 indicates a check bit failure.  32 indicates checkbit cx
 *	                                       33 indicates checkbit c0
 *	                                       34 indicates checkbit c1
 *	                                       35 indicates checkbit c2
 *	                                       36 indicates checkbit c4
 *	                                       37 indicates checkbit c8
 *	                                       38 indicates checkbit c16
 */

#define	EDCCX	32			/* checkbit cx  is a 1-bit error */
#define	EDCC0	(EDCCX+1)		/* checkbit c0  is a 1-bit error */
#define	EDCC1	(EDCCX+2)		/* checkbit c1  is a 1-bit error */
#define	EDCC2	(EDCCX+3)		/* checkbit c2  is a 1-bit error */
#define	EDCC4	(EDCCX+4)		/* checkbit c4  is a 1-bit error */
#define	EDCC8	(EDCCX+5)		/* checkbit c8  is a 1-bit error */
#define	EDCC16	(EDCCX+6)		/* checkbit c16 is a 1-bit error */
#define	EDCTWO	(EDCC16+1)		/* 2-bit error */
#define	EDCMLT	(EDCTWO+1)		/* 3 or more bits in error */
#define	EDCOK	(-1)			/* no error */

static	u_char	sgs_mem_decode[] = {
	EDCOK,	EDCCX,	EDCC0,	EDCTWO,	EDCC1,	EDCTWO,	EDCTWO,	EDCMLT,
	EDCC2,	EDCTWO,	EDCTWO,	17,	EDCTWO,	EDCMLT,	16,	EDCTWO,
	EDCC4,	EDCTWO,	EDCTWO,	18,	EDCTWO,	19,	20,	EDCTWO,
	EDCTWO,	21,	22,	EDCTWO,	23,	EDCTWO,	EDCTWO,	EDCMLT,
	EDCC8,	EDCTWO,	EDCTWO,	8,	EDCTWO,	9,	10,	EDCTWO,
	EDCTWO,	11,	12,	EDCTWO,	13,	EDCTWO,	EDCTWO,	EDCMLT,
	EDCTWO,	14,	EDCMLT,	EDCTWO,	15,	EDCTWO,	EDCTWO,	EDCMLT,
	EDCMLT,	EDCTWO,	EDCTWO,	EDCMLT,	EDCTWO,	EDCMLT,	EDCMLT,	EDCTWO,
	EDCC16,	EDCTWO,	EDCTWO,	EDCMLT,	EDCTWO,	EDCMLT,	EDCMLT,	EDCTWO,
	EDCTWO,	EDCMLT,	1,	EDCTWO,	EDCMLT,	EDCTWO,	EDCTWO,	0,
	EDCTWO,	EDCMLT,	2,	EDCTWO,	3,	EDCTWO,	EDCTWO,	4,
	5,	EDCTWO,	EDCTWO,	6,	EDCTWO,	7,	EDCMLT,	EDCTWO,
	EDCTWO,	EDCMLT,	24,	EDCTWO,	25,	EDCTWO,	EDCTWO,	26,
	27,	EDCTWO,	EDCTWO,	28,	EDCTWO,	29,	EDCMLT,	EDCTWO,
	30,	EDCTWO,	EDCTWO,	EDCMLT,	EDCTWO,	31,	EDCMLT,	EDCTWO,
	EDCTWO,	EDCMLT,	EDCMLT,	EDCTWO,	EDCMLT,	EDCTWO,	EDCTWO,	EDCMLT
};

#define LO_EDC_SE_ME	(MEM_EDC_LO_SE | MEM_EDC_LO_ME)
#define HI_EDC_SE_ME	(MEM_EDC_HI_SE | MEM_EDC_HI_ME)

static
sgs_mem_poll(cd)
	register struct	ctlr_desc *cd;
{
	register u_long	addr;
	u_int	bank;
	u_char	edc;
	int	errbit;
	spl_t	s;
	bool_t	normal_cycle;
	bool_t	xover_cycle;
	u_char	synd_lo;
	u_char	synd_hi;
	static	char	Local[] = "local (refresh/scrub)";
	static	char	Normal[] = "normal bus";
	static	char	XMessage[] = "Low EDC status reflects cross over status\n";
	static	char	Edc[] =
		"%s %d: %s %s error, %s cycle.\nbank=%d addr=0x%x error status=0x%x synd=0x%x\n";
	static	char	Interpret_Edc[] =
		"%s %d: single bit error on %s bit %d\n";
	static	char	Hard[] = "Non-correctable";
	static	char	Soft[] = "Correctable";
	/*
	 * These messages are for both rev 0 and rev 1 (or later) of
	 * the symmetry memory board.  The definitions of the overflow bits
	 * changed for these revisions.
	 */
	static	char	Hiovfl0[] = "HIGH overflow bit set, multiple errors have occured.\n";
	static	char	LOovfl0[] = "LOW overflow bit set, multiple errors have occured.\n";
	static	char	Hiovfl1[] = "multiple bit error overflow occured.\n";
	static	char	LOovfl1[] = "single bit error overflow occured.\n";

	/*
	 * Read EDC Error Register.  If zero, no errors.
	 */

	s = splhi();

	edc = rdslave(cd->cd_slic, MEM_EDC) & MEM_EDC_MASK;

	/*
	 * If any errors, read the syndrome registers
	 */

	if (edc) {
		synd_lo = rdslave(cd->cd_slic, MEM_SYND_LO) & MEM_SYND_MASK;
		synd_hi = rdslave(cd->cd_slic, MEM_SYND_HI) & MEM_SYND_MASK;
	}

	/*
	 * Check low part of EDC error register, then high part.
	 */

	if (edc & LO_EDC_SE_ME) {			/* low edc */
		addr = sgs_mem_err_addr(cd->cd_slic, MEM_BDP_LO);
		bank = sgs_mem_bank_decode(cd, addr);
		normal_cycle = synd_lo & MEM_NORMCY;
		xover_cycle = synd_hi & MEM_XOVER;
		printf(Edc, cd->cd_name, cd->cd_i,
			(edc & MEM_EDC_LO_ME) ? Hard : Soft,
			"EDC LO", normal_cycle ? Normal : Local,
			bank, addr, edc, synd_lo);
                /*
                 *+ A memory error has occurred.  If the error is
                 *+ soft, no corrective action has been performed.  If
                 *+ the error is hard, contact service.
                 */
		if (xover_cycle && (cd->cd_nbdps == 2)) 
			CPRINTF(XMessage);
		/*
		 * Report failing bit if single-bit error and not a
		 * multi-bit error.
		 */
		if ((edc & MEM_EDC_LO_SE) && !(edc & MEM_EDC_LO_ME)) {
			errbit = sgs_mem_decode[synd_lo & MEM_SYND_BITS];
			CPRINTF(Interpret_Edc, cd->cd_name, cd->cd_i,
				(errbit <= 31) ? "data" : "check",
				(errbit <= 31) ? errbit : errbit - 32);
		}
	}

	if (edc & HI_EDC_SE_ME) {			/* high edc */
		/*
		 * Note: address stored only in the low BDP.
		 */
		addr = sgs_mem_err_addr(cd->cd_slic, MEM_BDP_LO);
		bank = sgs_mem_bank_decode(cd, addr);
		xover_cycle =  synd_hi & MEM_XOVER;
		normal_cycle = synd_lo & MEM_NORMCY;
		printf(Edc, cd->cd_name, cd->cd_i,
			(edc & MEM_EDC_HI_ME) ? Hard : Soft,
			"EDC HI", normal_cycle ? Normal : Local,
			bank, addr, edc, synd_hi);
                /*
                 *+ A memory error has occurred.  If the error is
                 *+ soft, no corrective action has been performed.  If
                 *+ the error is hard, contact service.
                 */
		if (xover_cycle && (cd->cd_nbdps == 2)) 
			CPRINTF(XMessage);
		/*
		 * Report failing bit if single-bit error and not a
		 * multi-bit error.
		 */
		if ((edc & MEM_EDC_HI_SE) && !(edc & MEM_EDC_HI_ME)) {
			errbit = sgs_mem_decode[synd_hi & MEM_SYND_BITS];
			CPRINTF(Interpret_Edc, cd->cd_name, cd->cd_i,
				(errbit <= 31) ? "data" : "check",
				(errbit <= 31) ? errbit : errbit - 32);
		}
	}
	if (cd->cd_hrev == 0) {
		if (edc & MEM_EDC_LO_OV)
			CPRINTF(LOovfl0);
		if (edc & MEM_EDC_HI_OV)
			CPRINTF(Hiovfl0);
	} else {
		if (edc & MEM_EDC_HI_OV)
			CPRINTF(Hiovfl1);
		else if (edc & MEM_EDC_LO_OV)
			CPRINTF(LOovfl1);
	}

	/*
	 * If there was an error, clear it.
	 * Writing anything to Clear EDC Error Information register
	 * clears all error bits.
	 */

	if (edc)
		wrslave(cd->cd_slic, MEM_CLR_EDC, 0xbb);

	splx(s);
}

static
sgs_mem_err_addr(slic, bdp)
	u_char	slic;
	u_char	bdp;
{
	register u_int	temp;
	register u_int	result;
	register int	i;

	for (result = i = 0; i < 4; i++) {
		temp = rdSubslave(slic, bdp,
				   (u_char)(BDP_WAR|(BDP_BYTE0 - i))) & 0xff;
		result |= temp << (i*8);
	}
	return (result);
}

/*
 * sgs_mem_bank_decode()
 *	Return indication of which bank the error was in.
 *
 * A painful decode of cases.
 */

static
sgs_mem_bank_decode(cd, err_addr)
	register struct	ctlr_desc *cd;
	register u_long err_addr;
{
	u_int	bank_select;
	bool_t	normal_cycle = (rdslave(cd->cd_slic, MEM_SYND_LO) & MEM_NORMCY);
	bool_t	four_banks = (rdslave(cd->cd_slic, MEM_CFG) & MEM_CFG_4_BANKS);
	bool_t	half_banks = ((cd->cd_m_type&SL_M2_RAM_POP) == SL_M2_RAM_HALF);
	bool_t	four_meg_rams = ((cd->cd_m_type&SL_M2_RAM_DENS) == SL_M2_RAM_4MB
					|| ((cd->cd_m_expid & MEM_EXP_4MB) &&
					    (cd->cd_m_expid != MEM_EXP_NONE)));
	bool_t	interleaved = cd->cd_m_ileave;

#define BIT(i)	((err_addr) & (1 << (i)))

	if (half_banks) {
		if (!normal_cycle) {
				bank_select = (BIT(25)|BIT(26)|BIT(27)) >> 25;
		} else if (four_meg_rams) {
			if (interleaved)
				bank_select = (BIT(24) >> 24)
					    | ((BIT(26)|BIT(27)) >> 25);
			else
				bank_select = (BIT(24)|BIT(25)|BIT(26)) >> 24;
		} else if (interleaved) {
			if (four_banks)
				bank_select = (BIT(22) >> 22) | (BIT(24) >> 23);
			else
				bank_select = (BIT(22) >> 22)
					    | ((BIT(24)|BIT(25)) >> 23);
		} else if (four_banks)
			bank_select = (BIT(22)|BIT(23)) >> 22;
		else
			bank_select = (BIT(22)|BIT(23)|BIT(24)) >> 22;
	} else {
		if (!normal_cycle) {
				bank_select = (BIT(25)|BIT(26)|BIT(27)) >> 25;
		} else if (four_meg_rams) {
			if (interleaved)
				bank_select = (BIT(26)|BIT(27)|BIT(28)) >> 26;
			else
				bank_select = (BIT(25)|BIT(26)|BIT(27)) >> 25;
		} else if (interleaved) {
			if (four_banks)
				bank_select = (BIT(24)|BIT(25)) >> 24;
			else
				bank_select = (BIT(24)|BIT(25)|BIT(26)) >> 24;
		} else if (four_banks)
			bank_select = (BIT(23)|BIT(24)) >> 23;
		else
			bank_select = (BIT(23)|BIT(24)|BIT(25)) >> 23;
	} 

	return(bank_select);

#undef	BIT
}

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

#ifdef	RCS
static char rcsid[] = "$Header: buildcfg.c 1.11 90/10/12 $";
#endif

/*
 * buildcfg.c
 *	Routines to build SGS firmware style config tables needed
 *	when running on pre-SGS firmware.
 */

#include <sys/types.h>
#include <machine/cfg.h>
#include <machine/slicreg.h>
#include "sec.h"

#define	NULL 0
#define	K 1024
#define	CFG_SIZE (CD_STAND_ADDR - (int)CD_LOC)	/* SGS configuration tables */
#define	TOC_SIZE 256				/* number of board types */
caddr_t	mem_alloc();				/* allocates config memory */
static caddr_t memp;				/* memory pointer for above */

	/* MACROS to make the code neater */
#define	BOOTP(p)	((struct cfg_boot   *)(p))
#define PROCP(p)	((struct cfg_proc   *)(p))
#define MMEMP(p)	((struct cfg_mmem   *)(p))
#define SCSIP(p)	((struct cfg_scsi   *)(p))
#define MBADP(p)	((struct cfg_mbad   *)(p))
#define UNKNP(p)	((struct cfg_unkn   *)(p))
#define CLKAP(p)	((struct cfg_clkarb *)(p))
#define ZDCP(p) 	((struct cfg_zdc    *)(p))

	/* per-controller information */
struct ctlr_type_desc proc032_ctlr_type_desc = {
	"PROC/032", SLB_PROCBOARD, CTP_LAST,
	0,	0,	0,	0,	0,	0,
	CTF_BINIT|CTF_NEEDMEM|CTF_ISPROC|CTF_LOWPRI,
};
struct ctlr_type_desc prock20_ctlr_type_desc = {
	"PROC/K20", SLB_KXXBOARD, CTP_LAST,
	0,	0,	0,	0,	0,	0,
	CTF_BINIT|CTF_NEEDMEM|CTF_ISPROC|CTF_LOWPRI,
};
struct ctlr_type_desc mem1_ctlr_type_desc = {
	"MEM", SLB_MEMBOARD, CTP_FIRST,
	0,	0,	0,	0,	0,	0,
	CTF_BRESP|CTF_ISMEM|CTF_STD_HEADER|CTF_COMPAT,
};
struct ctlr_type_desc sced_ctlr_type_desc = {
	"SCED", SLB_SCSIBOARD, CTP_MIDDLE,
	0,	0,	0,	0,	0,	0,
	CTF_BINIT|CTF_NEEDMEM|CTF_STD_HEADER|CTF_HIPRI,
};
struct ctlr_type_desc mbad_ctlr_type_desc = {
	"MBAD", SLB_MBABOARD, CTP_MIDDLE,
	0,	0,	0,	0,	0,	0,
	CTF_BINIT|CTF_BRESP|CTF_STD_HEADER|CTF_HIPRI,
};
struct ctlr_type_desc cadm_ctlr_type_desc = {
	"CADM", SLB_CLKARBBOARD, CTP_MIDDLE,
	0,	0,	0,	0,	0,	0,
	CTF_BINIT|CTF_NEEDMEM|CTF_STD_HEADER|CTF_LOWPRI,
};
struct ctlr_type_desc dcc_ctlr_type_desc = {
	"ZDC", SLB_ZDCBOARD, CTP_MIDDLE,
	0,	0,	0,	0,	0,	0,
	CTF_BINIT|CTF_NEEDMEM|CTF_STD_HEADER|CTF_HIPRI,
};
struct ctlr_type_desc oem_ctlr_type_desc = {
	"UNKN", 0xff, CTP_MIDDLE,
	0,	0,	0,	0,	0,	0,
	CTF_BINIT|CTF_BRESP|CTF_ISOEM|CTF_STD_HEADER,
};

	/* Tables for slot priorities */
static u_char slotpri20[] = {
	SYSPRI_LOW,
	SYSPRI_LOW, SYSPRI_LOW, SYSPRI_LOW, SYSPRI_LOW, SYSPRI_LOW,
	SYSPRI_LOW, SYSPRI_LOW, SYSPRI_LOW, SYSPRI_LOW, SYSPRI_LOW,
	SYSPRI_HIGH, SYSPRI_HIGH, SYSPRI_HIGH, SYSPRI_HIGH, SYSPRI_HIGH,
	SYSPRI_BOTH, SYSPRI_BOTH, SYSPRI_BOTH, SYSPRI_BOTH, SYSPRI_BOTH
};
static u_char slotpri12[] = {
	SYSPRI_NONE,
	SYSPRI_LOW, SYSPRI_LOW, SYSPRI_LOW,
	SYSPRI_LOW, SYSPRI_LOW, SYSPRI_LOW,
	SYSPRI_HIGH, SYSPRI_HIGH, SYSPRI_HIGH,
	SYSPRI_HIGH, SYSPRI_HIGH, SYSPRI_HIGH
};

/*
 * buildcfg
 *
 * Build new configuration tables (if pre-SGS firmware).
 * Errors are flagged by printf (no status is returned).
 */
buildcfg()
{
	struct cfg_ptr *op = (struct cfg_ptr *)CFG_PTR;	/* old tables */
	register struct config_desc *np = CD_LOC;	/* new tables */
	register struct cfg_com *cp;
	register struct ctlr_desc *dp, *pp;
	int memsize, maxmem, version, count, console;
	int nproc, nmmem, nscsi, nmbad, nunkn, nclkarb, nzdc;
	int mem_type, mem_exp, mem_rows;

	if (np->c_version)
		return;
	version = op->head_cfg->b_cfg_version;
	if (version == 0) {
		printf("ERROR: Version 0 cfg.h structures not supported.\n");
		return;
	}
	if (op->head_cfg->cfg_com.c_diag_flag & CFG_NEW_CFG)
		return;				/* tables already built */
	/*
	 * Running on pre-SGS firmware so create a 
	 * fake cfg table using the old info as best we can.
	 */
	memsize = maxmem = 0;
	nproc = nmmem = nscsi = nmbad = nunkn = nclkarb = nzdc = 0;
	bzero((char *)np, CFG_SIZE);		/* start with zero'ed memory */
	memp = (caddr_t)(&np[1]);		/* start allocating mem here */
	np->c_sys.sd_type = SYSTYP_B8;		/* assume B8000 */
	np->c_sys_mode.sm_bus_mode = CD_BUS_COMPAT;
	np->c_sys_mode.sm_tsize = 8;		/* transfer size is 8 */
	np->c_sys_mode.sm_bsize = 16;		/* cache block size is 16 */

	cp = (struct cfg_com *) op->head_cfg;
	while (cp != NULL) {

		switch (cp->c_type) {

		case CFG_BOOT:
			np->c_version = CD_VERSION;
			np->c_flags = cp->c_diag_flag;
			np->c_boot_flag = BOOTP(cp)->b_boot_flag;
			np->c_bname_size = BNAMESIZE;
			np->c_boot_name = mem_alloc((unsigned)np->c_bname_size);
			bcopy(BOOTP(cp)->b_boot_name, 
				np->c_boot_name, (unsigned)np->c_bname_size);
			np->c_pagetable = BOOTP(cp)->b_pagetables;
			np->c_clock_rate = BOOTP(cp)->b_clock_rate;
			console = BOOTP(cp)->b_console;
			np->c_kill = BOOTP(cp)->b_kill;
			np->c_erase = BOOTP(cp)->b_erase;
			np->c_interrupt = BOOTP(cp)->b_interrupt;
			np->c_bottom = (int)BOOTP(cp)->b_bottom;
			count = sizeof(BOOTP(cp)->b_mmap);
			np->c_mmap = (u_long *) mem_alloc((unsigned)count);
			bcopy((char *)BOOTP(cp)->b_mmap, 
			      (char *)np->c_mmap, (unsigned)count);
			np->c_toc_size = TOC_SIZE;
			np->c_toc = (struct ctlr_toc *)
				mem_alloc(TOC_SIZE * sizeof(struct ctlr_toc));
			count = BOOTP(cp)->b_num_proc + BOOTP(cp)->b_num_mmem +
				BOOTP(cp)->b_num_scsi + BOOTP(cp)->b_num_mbad +
				BOOTP(cp)->b_num_unkn;
			if (version >= 2) {
				count += BOOTP(cp)->b_num_clkarb +
				  	 BOOTP(cp)->b_num_zdc;
			}
			np->c_ctlrs = dp = (struct ctlr_desc *) 
				mem_alloc((unsigned)count * 
					sizeof(struct ctlr_desc));
			np->c_end_ctlrs = &np->c_ctlrs[count];
			break;

		case CFG_PROC:
			dp->cd_diag_flag = PROCP(cp)->cfg_com.c_diag_flag;
			dp->cd_slic = PROCP(cp)->pr_slic_addr;
			dp->cd_type = rdslave(dp->cd_slic, SL_G_BOARDTYPE);
			dp->cd_var = rdslave(dp->cd_slic, SL_G_VARIATION);
			dp->cd_hrev = PROCP(cp)->pr_hrev;
			dp->cd_srev = PROCP(cp)->pr_srev;
			dp->cd_i = nproc++;
			switch (dp->cd_type) {
			case SLB_PROCBOARD:
				copy_type_desc(dp, &proc032_ctlr_type_desc);
				break;
			case SLB_KXXBOARD:
				copy_type_desc(dp, &prock20_ctlr_type_desc);
				break;
			default:
				printf("ERROR: unknown CFG_PROC type 0x%x\n",
					dp->cd_type);
				break;
			}
			dp->cd_p_speed = np->c_clock_rate;
			dp->cd_p_fp = 0;		/* std floating pt. */
			dp->cd_p_width = 32;		/* 32 bit bus width */
			dp->cd_p_nsets = 2;		/* 2 cache sets */
			dp->cd_p_setsize = 4;		/* w/ 4K per set */
			add_toc(dp, (u_char)op->head_cfg->b_num_proc);
			dp++;
			break;

		case CFG_MMEM:
			dp->cd_diag_flag = MMEMP(cp)->cfg_com.c_diag_flag;
			dp->cd_slic = MMEMP(cp)->me_slic_addr;
			dp->cd_type = rdslave(dp->cd_slic, SL_G_BOARDTYPE);
			dp->cd_var = rdslave(dp->cd_slic, SL_G_VARIATION);
			dp->cd_hrev = MMEMP(cp)->me_hrev;
			dp->cd_srev = MMEMP(cp)->me_srev;
			dp->cd_i = nmmem++;
			copy_type_desc(dp, &mem1_ctlr_type_desc);
			dp->cd_m_psize = MMEMP(cp)->me_size;
			dp->cd_m_lsize = dp->cd_m_psize;
			dp->cd_m_round = dp->cd_m_psize;
			dp->cd_m_base = MMEMP(cp)->me_start_addr;
			mem_type = rdslave(dp->cd_slic, SL_M_BSIZE);
			mem_exp = rdslave(dp->cd_slic, SL_M_EXP);
			mem_rows = ((~mem_type & SLB_LSIZE) >> 4) +
				   ((~mem_exp  & SLB_RSIZE) >> 4);
			dp->cd_m_pbanks = (1 << mem_rows) - 1;
			dp->cd_m_lbanks = dp->cd_m_pbanks;
			dp->cd_m_ileave = MMEMP(cp)->me_interleave;
			dp->cd_m_type = MMEMP(cp)->me_type;
			dp->cd_m_width = 32;
			add_toc(dp, (u_char)op->head_cfg->b_num_mmem);
			dp++;
			break;

		case CFG_SCSI:
			dp->cd_diag_flag = SCSIP(cp)->cfg_com.c_diag_flag;
			dp->cd_slic = SCSIP(cp)->sc_slic_addr;
			dp->cd_type = rdslave(dp->cd_slic, SL_G_BOARDTYPE);
			dp->cd_var = rdslave(dp->cd_slic, SL_G_VARIATION);
			dp->cd_hrev = SCSIP(cp)->sc_hrev;
			dp->cd_srev = SCSIP(cp)->sc_srev;
			dp->cd_i = nscsi++;
			copy_type_desc(dp, &sced_ctlr_type_desc);
			dp->cd_sc_init_queue = SCSIP(cp)->sc_init_queue;
			dp->cd_sc_version = SCSIP(cp)->sc_version;
			dp->cd_sc_host_num = SCSIP(cp)->sc_host_num;
			bcopy((char *)SCSIP(cp)->sc_enet_addr, 
			      (char *)dp->cd_sc_enet_addr,
			      sizeof(dp->cd_sc_enet_addr));
			/*
			 * Map old constants for console into new ones.
			 */
			if (SCSIP(cp)->sc_is_console) {
				np->c_cons = dp;
				switch (console) {
				case CON_LOCAL:	
					dp->cd_sc_cons = CDSC_LOCAL;
					break;
				case CON_REMOTE:
					dp->cd_sc_cons = CDSC_REMOTE;
					break;
				case CON_ETHER:
					dp->cd_sc_cons = CDSC_AUX;
					break;
				}
			} else
				dp->cd_sc_cons = CDSC_NOT_CONS;
			add_toc(dp, (u_char)op->head_cfg->b_num_scsi);
			dp++;
			break;

		case CFG_MBAD:
			dp->cd_diag_flag = MBADP(cp)->cfg_com.c_diag_flag;
			dp->cd_slic = MBADP(cp)->mb_slic_addr;
			dp->cd_type = rdslave(dp->cd_slic, SL_G_BOARDTYPE);
			dp->cd_var = rdslave(dp->cd_slic, SL_G_VARIATION);
			dp->cd_hrev = MBADP(cp)->mb_hrev;
			dp->cd_srev = MBADP(cp)->mb_srev;
			dp->cd_i = nmbad++;
			copy_type_desc(dp, &mbad_ctlr_type_desc);
			dp->cd_mb_version = MBADP(cp)->mb_version;
			add_toc(dp, (u_char)op->head_cfg->b_num_mbad);
			dp++;
			break;

		case CFG_CLKARB:
			/* Must be a B21000 so fix up type and slots */
			np->c_sys.sd_type = SYSTYP_B21;		/* B21000 */
			dp->cd_diag_flag = CLKAP(cp)->cfg_com.c_diag_flag;
			dp->cd_slic = CLKAP(cp)->clk_slic_addr;
			dp->cd_type = rdslave(dp->cd_slic, SL_G_BOARDTYPE);
			dp->cd_var = rdslave(dp->cd_slic, SL_G_VARIATION);
			dp->cd_hrev = CLKAP(cp)->clk_hrev;
			dp->cd_srev = CLKAP(cp)->clk_srev;
			dp->cd_i = nclkarb++;
			dp->cd_ca_sysid = CLKAP(cp)->clk_sysid;
			dp->cd_ca_fptype = CLKAP(cp)->clk_fptype;
			if (dp->cd_diag_flag & CFG_C_OPT_PRI)
				np->c_sys_mode.sm_opt_low = 1;
			copy_type_desc(dp, &cadm_ctlr_type_desc);
			add_toc(dp, (u_char)op->head_cfg->b_num_clkarb);
			dp++;
			break;

		case CFG_ZDC:
			dp->cd_diag_flag = ZDCP(cp)->cfg_com.c_diag_flag;
			dp->cd_slic = ZDCP(cp)->zdc_slic_addr;
			dp->cd_type = rdslave(dp->cd_slic, SL_G_BOARDTYPE);
			dp->cd_var = rdslave(dp->cd_slic, SL_G_VARIATION);
			dp->cd_hrev = ZDCP(cp)->zdc_hrev;
			dp->cd_srev = ZDCP(cp)->zdc_srev;
			dp->cd_i = nzdc++;
			dp->cd_dc_version = ZDCP(cp)->zdc_version;
			copy_type_desc(dp, &dcc_ctlr_type_desc);
			add_toc(dp, (u_char)op->head_cfg->b_num_zdc);
			dp++;
			break;

		default:
			printf("WARNING: unknown board type 0x%x\n",cp->c_type);

			/* fall into ... */

		case CFG_UNKN:
			dp->cd_diag_flag = UNKNP(cp)->cfg_com.c_diag_flag;
			dp->cd_slic = UNKNP(cp)->un_slic_addr;
			dp->cd_type = rdslave(dp->cd_slic, SL_G_BOARDTYPE);
			dp->cd_var = rdslave(dp->cd_slic, SL_G_VARIATION);
			dp->cd_hrev = UNKNP(cp)->un_hrev;
			dp->cd_srev = UNKNP(cp)->un_srev;
			dp->cd_i = nunkn++;
			copy_type_desc(dp, &oem_ctlr_type_desc);
			if (dp->cd_type == SLB_GFDEBOARD)
				dp->cd_ct->ct_flags |= CTF_LOWPRI;
			/* THIS IS NOT COMPLETE SINCE FW DOES NOT SORT UNKN's */
			add_toc(dp, (u_char)op->head_cfg->b_num_unkn);
			dp++;
			break;

		}
		cp = (struct cfg_com *)cp->c_next;
	}
	/* XXX: fix up toc for unknown board types */

	/*
	 * Now figure out several things related to memory boards, like
	 * max memory, max memory address, and memory board interleave
	 * by looping through all memory controllers.
	 */
	dp = &np->c_ctlrs[np->c_toc[SLB_MEMBOARD].ct_start];
	count = np->c_toc[SLB_MEMBOARD].ct_count;
	while (count-- > 0) {
		if (FAILED(dp)) {
			dp++;
			continue;
		}
		memsize += dp->cd_m_psize;
		if (dp->cd_m_ileave == 0) {
			dp->cd_m_ileave = NO_ILEAVE;
			if (dp->cd_m_base + dp->cd_m_psize > maxmem)
				maxmem = dp->cd_m_base + dp->cd_m_psize;
		} else {
			/* 
			 * First interleaved board seen at a given
			 * base address is ILEAVE_LO, second is at ILEAVE_HI.
			 */
			pp = &np->c_ctlrs[np->c_toc[SLB_MEMBOARD].ct_start];
			while (pp < dp) {
				if ( pp->cd_m_base == dp->cd_m_base ) {
					dp->cd_m_ileave = ILEAVE_HI;
					break;
				}
				pp++;
			}
			if (pp == dp)
				dp->cd_m_ileave = ILEAVE_LO;
			if (dp->cd_m_base + (2 * dp->cd_m_psize) > maxmem)
				maxmem = dp->cd_m_base + (2 * dp->cd_m_psize);
		}
		dp++;
	}
	/*
	 * Fill out per-slot priority table based on System type.
	 */
	switch (np->c_sys.sd_type) {
	case SYSTYP_B8:
		np->c_sys.sd_nslots = 12;		/* 12 slots */
		np->c_sys.sd_slotpri =			/* per slot priority */
			(u_char *)mem_alloc(sizeof(slotpri12));
		bcopy((char *)slotpri12, 
			(char *)np->c_sys.sd_slotpri, sizeof(slotpri12));
		break;
	case SYSTYP_B21:
		np->c_sys.sd_nslots = 20;		/* 20 slots */
		np->c_sys.sd_slotpri = 			/* per slot priority */
			(u_char *)mem_alloc(sizeof(slotpri20));
		bcopy((char *)slotpri20, 
			(char *)np->c_sys.sd_slotpri, sizeof(slotpri20));
		break;
	}
	np->c_memsize = memsize / (1 * K);	/* memory size in KB's */
	np->c_maxmem = maxmem;			/* last memory addr + 1 */
	np->c_mmap_size = maxmem / (512 * K);	/* 1 bit = 512K */
}

/*
 * Added an entry to the controller 
 * Table Of Contents (c_toc array).
 */
add_toc(dp, n)
	struct ctlr_desc *dp;
	u_char n;
{
	register u_char type = dp->cd_type;
	register struct config_desc *np = CD_LOC;

	if (np->c_toc[type].ct_count != 0)
		return;
	np->c_toc[type].ct_start = dp - np->c_ctlrs;
	np->c_toc[type].ct_count = n;
}

/*
 * Build a controller type description entry.
 */
copy_type_desc(dp, cp)
	struct ctlr_desc *dp;
	struct ctlr_type_desc *cp;
{
	struct config_desc *np = CD_LOC;
	int nbytes;
	/*
	 * If last controller type is the same as the current 
	 * controller type, save space by sharing the entry.
	 */
	if (dp != np->c_ctlrs && dp[-1].cd_type == cp->ct_type) {
		dp->cd_ct = dp[-1].cd_ct;
		return;
	}
	dp->cd_ct = (struct ctlr_type_desc *)
		mem_alloc(sizeof(struct ctlr_type_desc));
	bcopy((char *)cp, (char *)dp->cd_ct, sizeof(struct ctlr_type_desc));
	nbytes = strlen(cp->ct_name) + 1;
	dp->cd_ct->ct_name = mem_alloc((unsigned)nbytes);
	bcopy(cp->ct_name, dp->cd_ct->ct_name, (unsigned)nbytes);
}

caddr_t
mem_alloc(size)
	unsigned int size;
{
	caddr_t value;

	if (size == 0) {
		printf("ERROR: mem_alloc called with size = 0x%x\n", size);
		return ((caddr_t) NULL);
	}
	if (size & ~3)
		memp = (caddr_t) (((int)memp + 3) & ~3);
	value = memp;
	memp += size;
	if ((int)memp >= 16*K)
		printf("ERROR: mem_alloc out of memory!\n");
	return (value);
}

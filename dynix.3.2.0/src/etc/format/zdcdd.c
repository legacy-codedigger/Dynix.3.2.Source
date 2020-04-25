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
 * ident	"$Header: zdcdd.c 1.2 90/03/17 $"
 */

/* $Log:	zdcdd.c,v $
 */

#include <sys/types.h>
#include <sys/param.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/file.h>
#ifdef BSD
#include <sys/fs.h>
#include <sys/ioctl.h>
#include <zdc/zdc.h>
#include <zdc/zdbad.h>
#include <zdc/zdioctl.h>
#else
#include <sys/ufsfilsys.h>
#include <sys/zdc.h>
#include <sys/zdbad.h>
#include <sys/zdioctl.h>
#endif
#include <diskinfo.h>
#include "format.h"
#include "zdformat.h"

static char format[] = "\t\"%s\" on disk is 0x%x should be 0x%x\n";

print_zdcdd(d, z)
struct zdcdd *d;
struct zdcdd *z;
{
	if (d->zdd_magic != z->zdd_magic)
	    fprintf (stderr, format, "mg", z->zdd_magic, d->zdd_magic);
	if (d->zdd_ecc_bytes != z->zdd_ecc_bytes)
	    fprintf (stderr, format, "ec", z->zdd_ecc_bytes, d->zdd_ecc_bytes);
	if (d->zdd_spare != z->zdd_spare)
	    fprintf (stderr, format, "sp", z->zdd_spare, d->zdd_spare);
	if (d->zdd_sectors != z->zdd_sectors)
	    fprintf (stderr, format, "se", z->zdd_sectors, d->zdd_sectors);
	if (d->zdd_tracks != z->zdd_tracks)
	    fprintf (stderr, format, "tr", z->zdd_tracks, d->zdd_tracks);
	if (d->zdd_cyls != z->zdd_cyls)
	    fprintf (stderr, format, "cy", z->zdd_cyls, d->zdd_cyls);
	if (d->zdd_xfer_rate != z->zdd_xfer_rate)
	    fprintf (stderr, format, "xf", z->zdd_xfer_rate, d->zdd_xfer_rate);
	if (d->zdd_runt != z->zdd_runt)
	    fprintf (stderr, format, "ru", z->zdd_runt, d->zdd_runt);
	if (d->zdd_chdelay != z->zdd_chdelay)
	    fprintf (stderr, format, "ch", z->zdd_chdelay, d->zdd_chdelay);
	if (d->zdd_hsdelay != z->zdd_hsdelay)
	    fprintf (stderr, format, "hs", z->zdd_hsdelay, d->zdd_hsdelay);
	if (d->zdd_hpo_rd_bc != z->zdd_hpo_rd_bc)
	    fprintf (stderr, format, "hr", z->zdd_hpo_rd_bc, d->zdd_hpo_rd_bc);
	if (d->zdd_hpo_fmt_bc != z->zdd_hpo_fmt_bc)
	    fprintf (stderr, format, "hf", z->zdd_hpo_fmt_bc, d->zdd_hpo_fmt_bc);
	if (d->zdd_cskew != z->zdd_cskew)
	    fprintf (stderr, format, "ck", z->zdd_cskew, d->zdd_cskew);
	if (d->zdd_tskew != z->zdd_tskew)
	    fprintf (stderr, format, "tk", z->zdd_tskew, d->zdd_tskew);
	if (d->zdd_hdr_bc != z->zdd_hdr_bc)
	    fprintf (stderr, format, "hb", z->zdd_hdr_bc, d->zdd_hdr_bc);
	if (d->zdd_sector_bc != z->zdd_sector_bc)
	    fprintf (stderr, format, "sb", z->zdd_sector_bc, d->zdd_sector_bc);
	if (d->zdd_strt_ign_bc != z->zdd_strt_ign_bc)
	    fprintf (stderr, format, "si", z->zdd_strt_ign_bc, d->zdd_strt_ign_bc);
	if (d->zdd_end_ign_bc != z->zdd_end_ign_bc)
	    fprintf (stderr, format, "ei", z->zdd_end_ign_bc, d->zdd_end_ign_bc);
	if (d->zdd_ddc_regs.dr_status != z->zdd_ddc_regs.dr_status)
	    fprintf (stderr, format, "ds", z->zdd_ddc_regs.dr_status, d->zdd_ddc_regs.dr_status);
	if (d->zdd_ddc_regs.dr_error != z->zdd_ddc_regs.dr_error)
	    fprintf (stderr, format, "de", z->zdd_ddc_regs.dr_error, d->zdd_ddc_regs.dr_error);
	if (d->zdd_ddc_regs.dr_ppb0 != z->zdd_ddc_regs.dr_ppb0)
	    fprintf (stderr, format, "p0", z->zdd_ddc_regs.dr_ppb0, d->zdd_ddc_regs.dr_ppb0);
	if (d->zdd_ddc_regs.dr_ppb1 != z->zdd_ddc_regs.dr_ppb1)
	    fprintf (stderr, format, "p1", z->zdd_ddc_regs.dr_ppb1, d->zdd_ddc_regs.dr_ppb1);
	if (d->zdd_ddc_regs.dr_ppb2 != z->zdd_ddc_regs.dr_ppb2)
	    fprintf (stderr, format, "p2", z->zdd_ddc_regs.dr_ppb2, d->zdd_ddc_regs.dr_ppb2);
	if (d->zdd_ddc_regs.dr_ppb3 != z->zdd_ddc_regs.dr_ppb3)
	    fprintf (stderr, format, "p3", z->zdd_ddc_regs.dr_ppb3, d->zdd_ddc_regs.dr_ppb3);
	if (d->zdd_ddc_regs.dr_ppb4 != z->zdd_ddc_regs.dr_ppb4)
	    fprintf (stderr, format, "p4", z->zdd_ddc_regs.dr_ppb4, d->zdd_ddc_regs.dr_ppb4);
	if (d->zdd_ddc_regs.dr_ppb5 != z->zdd_ddc_regs.dr_ppb5)
	    fprintf (stderr, format, "p5", z->zdd_ddc_regs.dr_ppb5, d->zdd_ddc_regs.dr_ppb5);
	if (d->zdd_ddc_regs.dr_ptb0 != z->zdd_ddc_regs.dr_ptb0)
	    fprintf (stderr, format, "t0", z->zdd_ddc_regs.dr_ptb0, d->zdd_ddc_regs.dr_ptb0);
	if (d->zdd_ddc_regs.dr_ptb1 != z->zdd_ddc_regs.dr_ptb1)
	    fprintf (stderr, format, "t1", z->zdd_ddc_regs.dr_ptb1, d->zdd_ddc_regs.dr_ptb1);
	if (d->zdd_ddc_regs.dr_ptb2 != z->zdd_ddc_regs.dr_ptb2)
	    fprintf (stderr, format, "t2", z->zdd_ddc_regs.dr_ptb2, d->zdd_ddc_regs.dr_ptb2);
	if (d->zdd_ddc_regs.dr_ptb3 != z->zdd_ddc_regs.dr_ptb3)
	    fprintf (stderr, format, "t3", z->zdd_ddc_regs.dr_ptb3, d->zdd_ddc_regs.dr_ptb3);
	if (d->zdd_ddc_regs.dr_ptb4 != z->zdd_ddc_regs.dr_ptb4)
	    fprintf (stderr, format, "t4", z->zdd_ddc_regs.dr_ptb4, d->zdd_ddc_regs.dr_ptb4);
	if (d->zdd_ddc_regs.dr_ptb5 != z->zdd_ddc_regs.dr_ptb5)
	    fprintf (stderr, format, "t5", z->zdd_ddc_regs.dr_ptb5, d->zdd_ddc_regs.dr_ptb5);
	if (d->zdd_ddc_regs.dr_ec_ctrl != z->zdd_ddc_regs.dr_ec_ctrl)
	    fprintf (stderr, format, "er", z->zdd_ddc_regs.dr_ec_ctrl, d->zdd_ddc_regs.dr_ec_ctrl);
	if (d->zdd_ddc_regs.dr_hbc != z->zdd_ddc_regs.dr_hbc)
	    fprintf (stderr, format, "hc", z->zdd_ddc_regs.dr_hbc, d->zdd_ddc_regs.dr_hbc);
	if (d->zdd_ddc_regs.dr_dc != z->zdd_ddc_regs.dr_dc)
	    fprintf (stderr, format, "dc", z->zdd_ddc_regs.dr_dc, d->zdd_ddc_regs.dr_dc);
	if (d->zdd_ddc_regs.dr_oc != z->zdd_ddc_regs.dr_oc)
	    fprintf (stderr, format, "oc", z->zdd_ddc_regs.dr_oc, d->zdd_ddc_regs.dr_oc);
	if (d->zdd_ddc_regs.dr_sc != z->zdd_ddc_regs.dr_sc)
	    fprintf (stderr, format, "sc", z->zdd_ddc_regs.dr_sc, d->zdd_ddc_regs.dr_sc);
	if (d->zdd_ddc_regs.dr_nso != z->zdd_ddc_regs.dr_nso)
	    fprintf (stderr, format, "ns", z->zdd_ddc_regs.dr_nso, d->zdd_ddc_regs.dr_nso);
	if (d->zdd_ddc_regs.dr_hb0_pat != z->zdd_ddc_regs.dr_hb0_pat)
	    fprintf (stderr, format, "h0", z->zdd_ddc_regs.dr_hb0_pat, d->zdd_ddc_regs.dr_hb0_pat);
	if (d->zdd_ddc_regs.dr_hb1_pat != z->zdd_ddc_regs.dr_hb1_pat)
	    fprintf (stderr, format, "h1", z->zdd_ddc_regs.dr_hb1_pat, d->zdd_ddc_regs.dr_hb1_pat);
	if (d->zdd_ddc_regs.dr_hb2_pat != z->zdd_ddc_regs.dr_hb2_pat)
	    fprintf (stderr, format, "h2", z->zdd_ddc_regs.dr_hb2_pat, d->zdd_ddc_regs.dr_hb2_pat);
	if (d->zdd_ddc_regs.dr_hb3_pat != z->zdd_ddc_regs.dr_hb3_pat)
	    fprintf (stderr, format, "h3", z->zdd_ddc_regs.dr_hb3_pat, d->zdd_ddc_regs.dr_hb3_pat);
	if (d->zdd_ddc_regs.dr_hb4_pat != z->zdd_ddc_regs.dr_hb4_pat)
	    fprintf (stderr, format, "h4", z->zdd_ddc_regs.dr_hb4_pat, d->zdd_ddc_regs.dr_hb4_pat);
	if (d->zdd_ddc_regs.dr_hb5_pat != z->zdd_ddc_regs.dr_hb5_pat)
	    fprintf (stderr, format, "h5", z->zdd_ddc_regs.dr_hb5_pat, d->zdd_ddc_regs.dr_hb5_pat);
	if (d->zdd_ddc_regs.dr_rdbc != z->zdd_ddc_regs.dr_rdbc)
	    fprintf (stderr, format, "rd", z->zdd_ddc_regs.dr_rdbc, d->zdd_ddc_regs.dr_rdbc);
	if (d->zdd_ddc_regs.dr_dma_addr != z->zdd_ddc_regs.dr_dma_addr)
	    fprintf (stderr, format, "dm", z->zdd_ddc_regs.dr_dma_addr, d->zdd_ddc_regs.dr_dma_addr);
	if (d->zdd_ddc_regs.dr_dpo_bc != z->zdd_ddc_regs.dr_dpo_bc)
	    fprintf (stderr, format, "do", z->zdd_ddc_regs.dr_dpo_bc, d->zdd_ddc_regs.dr_dpo_bc);
	if (d->zdd_ddc_regs.dr_hpr_bc != z->zdd_ddc_regs.dr_hpr_bc)
	    fprintf (stderr, format, "rh", z->zdd_ddc_regs.dr_hpr_bc, d->zdd_ddc_regs.dr_hpr_bc);
	if (d->zdd_ddc_regs.dr_hs1_bc != z->zdd_ddc_regs.dr_hs1_bc)
	    fprintf (stderr, format, "s1", z->zdd_ddc_regs.dr_hs1_bc, d->zdd_ddc_regs.dr_hs1_bc);
	if (d->zdd_ddc_regs.dr_hs2_bc != z->zdd_ddc_regs.dr_hs2_bc)
	    fprintf (stderr, format, "s2", z->zdd_ddc_regs.dr_hs2_bc, d->zdd_ddc_regs.dr_hs2_bc);
	if (d->zdd_ddc_regs.dr_hb0_ctrl != z->zdd_ddc_regs.dr_hb0_ctrl)
	    fprintf (stderr, format, "c0", z->zdd_ddc_regs.dr_hb0_ctrl, d->zdd_ddc_regs.dr_hb0_ctrl);
	if (d->zdd_ddc_regs.dr_hb1_ctrl != z->zdd_ddc_regs.dr_hb1_ctrl)
	    fprintf (stderr, format, "c1", z->zdd_ddc_regs.dr_hb1_ctrl, d->zdd_ddc_regs.dr_hb1_ctrl);
	if (d->zdd_ddc_regs.dr_hb2_ctrl != z->zdd_ddc_regs.dr_hb2_ctrl)
	    fprintf (stderr, format, "c2", z->zdd_ddc_regs.dr_hb2_ctrl, d->zdd_ddc_regs.dr_hb2_ctrl);
	if (d->zdd_ddc_regs.dr_hb3_ctrl != z->zdd_ddc_regs.dr_hb3_ctrl)
	    fprintf (stderr, format, "c3", z->zdd_ddc_regs.dr_hb3_ctrl, d->zdd_ddc_regs.dr_hb3_ctrl);
	if (d->zdd_ddc_regs.dr_hb4_ctrl != z->zdd_ddc_regs.dr_hb4_ctrl)
	    fprintf (stderr, format, "c4", z->zdd_ddc_regs.dr_hb4_ctrl, d->zdd_ddc_regs.dr_hb4_ctrl);
	if (d->zdd_ddc_regs.dr_hb5_ctrl != z->zdd_ddc_regs.dr_hb5_ctrl)
	    fprintf (stderr, format, "c5", z->zdd_ddc_regs.dr_hb5_ctrl, d->zdd_ddc_regs.dr_hb5_ctrl);
	if (d->zdd_ddc_regs.dr_extdecc_bc != z->zdd_ddc_regs.dr_extdecc_bc)
	    fprintf (stderr, format, "xd", z->zdd_ddc_regs.dr_extdecc_bc, d->zdd_ddc_regs.dr_extdecc_bc);
	if (d->zdd_ddc_regs.dr_exthecc_bc != z->zdd_ddc_regs.dr_exthecc_bc)
	    fprintf (stderr, format, "xh", z->zdd_ddc_regs.dr_exthecc_bc, d->zdd_ddc_regs.dr_exthecc_bc);
	if (d->zdd_ddc_regs.dr_hpo_wr_bc != z->zdd_ddc_regs.dr_hpo_wr_bc)
	    fprintf (stderr, format, "po", z->zdd_ddc_regs.dr_hpo_wr_bc, d->zdd_ddc_regs.dr_hpo_wr_bc);
	if (d->zdd_ddc_regs.dr_dpr_wr_bc != z->zdd_ddc_regs.dr_dpr_wr_bc)
	    fprintf (stderr, format, "pr", z->zdd_ddc_regs.dr_dpr_wr_bc, d->zdd_ddc_regs.dr_dpr_wr_bc);
	if (d->zdd_ddc_regs.dr_ds1_bc != z->zdd_ddc_regs.dr_ds1_bc)
	    fprintf (stderr, format, "d1", z->zdd_ddc_regs.dr_ds1_bc, d->zdd_ddc_regs.dr_ds1_bc);
	if (d->zdd_ddc_regs.dr_ds2_bc != z->zdd_ddc_regs.dr_ds2_bc)
	    fprintf (stderr, format, "d2", z->zdd_ddc_regs.dr_ds2_bc, d->zdd_ddc_regs.dr_ds2_bc);
	if (d->zdd_ddc_regs.dr_dpo_pat != z->zdd_ddc_regs.dr_dpo_pat)
	    fprintf (stderr, format, "op", z->zdd_ddc_regs.dr_dpo_pat, d->zdd_ddc_regs.dr_dpo_pat);
	if (d->zdd_ddc_regs.dr_hpr_pat != z->zdd_ddc_regs.dr_hpr_pat)
	    fprintf (stderr, format, "or", z->zdd_ddc_regs.dr_hpr_pat, d->zdd_ddc_regs.dr_hpr_pat);
	if (d->zdd_ddc_regs.dr_hs1_pat != z->zdd_ddc_regs.dr_hs1_pat)
	    fprintf (stderr, format, "a1", z->zdd_ddc_regs.dr_hs1_pat, d->zdd_ddc_regs.dr_hs1_pat);
	if (d->zdd_ddc_regs.dr_hs2_pat != z->zdd_ddc_regs.dr_hs2_pat)
	    fprintf (stderr, format, "a2", z->zdd_ddc_regs.dr_hs2_pat, d->zdd_ddc_regs.dr_hs2_pat);
	if (d->zdd_ddc_regs.dr_gap_bc != z->zdd_ddc_regs.dr_gap_bc)
	    fprintf (stderr, format, "gp", z->zdd_ddc_regs.dr_gap_bc, d->zdd_ddc_regs.dr_gap_bc);
	if (d->zdd_ddc_regs.dr_df != z->zdd_ddc_regs.dr_df)
	    fprintf (stderr, format, "df", z->zdd_ddc_regs.dr_df, d->zdd_ddc_regs.dr_df);
	if (d->zdd_ddc_regs.dr_ltr != z->zdd_ddc_regs.dr_ltr)
	    fprintf (stderr, format, "lt", z->zdd_ddc_regs.dr_ltr, d->zdd_ddc_regs.dr_ltr);
	if (d->zdd_ddc_regs.dr_rtr != z->zdd_ddc_regs.dr_rtr)
	    fprintf (stderr, format, "rt", z->zdd_ddc_regs.dr_rtr, d->zdd_ddc_regs.dr_rtr);
	if (d->zdd_ddc_regs.dr_sector_bc != z->zdd_ddc_regs.dr_sector_bc)
	    fprintf (stderr, format, "et", z->zdd_ddc_regs.dr_sector_bc, d->zdd_ddc_regs.dr_sector_bc);
	if (d->zdd_ddc_regs.dr_gap_pat != z->zdd_ddc_regs.dr_gap_pat)
	    fprintf (stderr, format, "gt", z->zdd_ddc_regs.dr_gap_pat, d->zdd_ddc_regs.dr_gap_pat);
	if (d->zdd_ddc_regs.dr_dfmt_pat != z->zdd_ddc_regs.dr_dfmt_pat)
	    fprintf (stderr, format, "fd", z->zdd_ddc_regs.dr_dfmt_pat, d->zdd_ddc_regs.dr_dfmt_pat);
	if (d->zdd_ddc_regs.dr_hpo_pat != z->zdd_ddc_regs.dr_hpo_pat)
	    fprintf (stderr, format, "ah", z->zdd_ddc_regs.dr_hpo_pat, d->zdd_ddc_regs.dr_hpo_pat);
	if (d->zdd_ddc_regs.dr_dpr_pat != z->zdd_ddc_regs.dr_dpr_pat)
	    fprintf (stderr, format, "ad", z->zdd_ddc_regs.dr_dpr_pat, d->zdd_ddc_regs.dr_dpr_pat);
	if (d->zdd_ddc_regs.dr_ds1_pat != z->zdd_ddc_regs.dr_ds1_pat)
	    fprintf (stderr, format, "r1", z->zdd_ddc_regs.dr_ds1_pat, d->zdd_ddc_regs.dr_ds1_pat);
	if (d->zdd_ddc_regs.dr_ds2_pat != z->zdd_ddc_regs.dr_ds2_pat)
	    fprintf (stderr, format, "r2", z->zdd_ddc_regs.dr_ds2_pat, d->zdd_ddc_regs.dr_ds2_pat);
}

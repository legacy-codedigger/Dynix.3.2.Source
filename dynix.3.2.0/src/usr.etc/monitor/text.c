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

#ifndef lint
static char rcsid[] = "$Header: text.c 2.2 87/04/10 $";
#endif

/*
 * text.c
 *
 * Data structures for the screen
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/vm.h>
#include <sys/dk.h>
#include <sys/flock.h>
#include "monitor.h"
#include "sslib.h"

char *percent[] = {
	"0    10    20    30    40    50    60    70    80    90    100",
	"0    20    40    60    80    100",
	"0  20  40  60  80  100"
};

char *hashes[] = {
	"+-----+-----+-----+-----+-----+-----+-----+-----+-----+------+",
	"+--+--+--+--+--+--+--+--+--+---+",
	"+-+-+-+-+-+-+-+-+-+--+"
};

int p_bar_len[] = {
	60, 30, 20
};

/* >>>>> Perhaps, for total, just use the stuff for 1 col, (it is the same) */

int tot_bar_len = 60;

extern struct dk dk_sum;
extern struct ether_stat es_sum;
extern struct vmmeter rate;	/* rate over last interval */
extern struct vmtotal total;
extern struct proc_stat ps;
extern unsigned pt_user;	/* total user time */
extern unsigned pt_sys;		/* total sys time */
extern unsigned pt_total;	/* total processor time */
extern unsigned fsreadhit, fswritehit; /* File Sys hit ratios */
extern unsigned avedirty, deficit;
extern struct flckinfo flckinfo;
extern struct flock_stats fl_stats;

struct sstat scr_text[] = {
  /*
   * Processor time stats
   *  pt	fmt	datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&pt_user,	"Total User Time",	"usr %", 1 },
  { {0,0},	"%5d",	&pt_sys,	"Total System Time",	"sys %", 1 },
  { {0,0},	"%5d",	&pt_total,	"Total Time",		"tot %", 1 },
  /*
   * Process stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&ps.processes,	"Number of Procs",	"procs", 1 },
  { {0,0},	"%5d",	&ps.running,	"Running Procs",	"on p",	1 },
  { {0,0},	"%5d",	&ps.runnable,	"Runnable Procs",	"runq",	1 },
  { {0,0},	"%5d",	&ps.fastwait,	"Fast Wait",		"wait",	1 },
  { {0,0},	"%5d",	&ps.sleeping,	"Sleeping Procs",	"sleep",1 },
  { {0,0},	"%5d",	&ps.swapped,	"Swapped Procs",	"swapped",1 },
  /*
   * Misc. System stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&rate.v_syscall, "System Calls",	"sysc", 1 },
  { {0,0},	"%5d",	&rate.v_swtch,	"Context Switches",	"csw",	1 },
  { {0,0},	"%5d",	&rate.v_intr,	"Interrupts",		"intr",	1 },
  { {0,0},	"%5d",	&rate.v_trap,	"Traps",		"traps", 1 },
  /*
   * Process creation stats (fork/vfork/exec)
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&rate.v_cntfork, "Forks",		"fork",	1 },
  { {0,0},	"%5d",	&rate.v_cntvfork, "Vforks",		"vfork", 1 },
  { {0,0},	"%5d",	&rate.v_cntexec, "Execs",		"exec", 1 },
  /*
   * Ether stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&es_sum.pktin,	"Packets In",		"pkt in", 1 },
  { {0,0},	"%5d",	&es_sum.pktout,	"Packets Out",		"pkt out", 1 },
  /*
   * Disk stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},"%5d",(unsigned *)&dk_sum.dk_xfer,"Disk Transfers","dk xf", 1 },
  { {0,0},"%5d",(unsigned *)&dk_sum.dk_blks, "Disk KBytes","dk KB", 1 },
  /*
   * TTY stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&rate.v_ttyin,	"TTY Chars In",		"ttyin", 1 },
  { {0,0},	"%5d",	&rate.v_ttyout,	"TTY Chars Out",	"ttyout", 1 },
  /*
   * Paging stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&rate.v_faults,	"Page Faults",		"pf",	1 },
  { {0,0},	"%5d",	&rate.v_pgrec,	"Page Reclaims",	"pg rec",1 },
  { {0,0},	"%5d",	&rate.v_pgdrec,	"Dirty Pg Recs",	"pgdrec",1 },
  { {0,0},	"%5d",	&rate.v_pgin,	"Page Ins",		"pgin", 1 },
  { {0,0},	"%5d",	&rate.v_pgpgin,	"Pages Paged In",	"ppgin", 1 },
  { {0,0},	"%5d",	&rate.v_pgout,	"Page Outs",		"pgout", 1 },
  { {0,0},	"%5d",	&rate.v_pgpgout,"Pages Paged Out",	"ppgout", 1 },
  /*
   * Swapping stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&rate.v_swpin,	"Swap Ins",		"sw in", 1 },
  { {0,0},	"%5d",	&rate.v_pswpin,	"Pages Swapped In",	"pswin", 1 },
  { {0,0},	"%5d",	&rate.v_swpout,	"Swap Outs",		"sw out", 1 },
  { {0,0},	"%5d",	&rate.v_pswpout,"Pages Swapped Out",	"pswout", 1 },
  /*
   * Memory Use stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0}, "%5d", (unsigned *)&deficit, "Deficit", "deficit", 1 },
  { {0,0}, "%5d", (unsigned *)&total.t_free, "Free Memory", "free", 1 },
  { {0,0}, "%5d", (unsigned *)&avedirty, "Dirty Memory", "dirty", 1 },
  { {0,0}, "%5d", (unsigned *)&total.t_vm,"Total Virtual Mem","tot vm",1 },
  { {0,0},"%5d",(unsigned *)&total.t_avm,"Active Virtual Mem","act vm",1 },
  { {0,0}, "%5d", (unsigned *)&total.t_rm, "Total Real Mem", "tot rm", 1 },
  { {0,0}, "%5d", (unsigned *)&total.t_arm,"Active Real Mem","act rm", 1 },
  /*
   * File System stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&rate.v_rcount,	"FS Blk Reads",		"fs brd", 1 },
  { {0,0},	"%5d",	&rate.v_wcount,	"FS Blk Writes",	"fs bwt", 1 },
  { {0,0},	"%5d",	&fsreadhit,	"FS Read Hit",		"fs rdh", 1 },
  { {0,0},	"%5d",	&fswritehit,	"FS Write Hit",		"fs wth", 1 },
  /*
   * Physical I/O stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	&rate.v_phyrc,	"Raw Reads",		"raw rd", 1 },
  { {0,0},	"%5d",	&rate.v_phywc,	"Raw Writes",		"raw wt", 1 },
  { {0,0},	"%5d",	&rate.v_phyr,	"Raw Read KB",		"raw rKB", 1 },
  { {0,0},	"%5d",	&rate.v_phyw,	"Raw Write KB",		"raw wKB", 1 },
  /*
   * File/Record locking stats
   *  pt		datav		verbose			terse	display
   */
  { {0,0},	"%5d",	(unsigned *)&flckinfo.reccnt,"Locks Used",		"lckcnt", 1 },
  { {0,0},	"%5d",	&fl_stats.lck_ut,"Percent Locks Used",	"%locks", 1 },
  { {0,0},	"%5d",	(unsigned *)&flckinfo.filcnt,"Files Locked",	"filcnt", 1 },
  { {0,0},	"%5d",	&fl_stats.fil_ut,"Percent Files Used",	"%files", 1 },
};

int n_text = sizeof(scr_text)/sizeof(scr_text[0]);

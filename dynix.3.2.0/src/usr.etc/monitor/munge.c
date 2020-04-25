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
static char rcsid[] = "$Header: munge.c 2.4 87/04/10 $";
#endif

/*
 * munge.c
 *
 * Routines here calculate the rates for all stats
 * collected and sum of these rates across disk and ether devices
 */

#include <sys/types.h>
#include <sys/vm.h>
#include <sys/time.h>
#include <sys/dk.h>
#include <machine/param.h>
#include <machine/clock.h>
#include <sys/flock.h>
#include <stdio.h>
#include "sslib.h"

extern unsigned fsreadhit, fswritehit; /* File Sys hit ratios */
extern unsigned avedirty, deficit;

/*
 * The so macro handles the difference of two unsigned's in the
 * presence of wraparound
 */
#define MAXUNSIGNED	4294967295
#define	so(n,o)		(((n) >= (o)) ? ((n)-(o)) : (MAXUNSIGNED-(o)+(n)+1))

munge(sc, vm_old, vm_new, vm_rate, dk_old, dk_new, dk_rate, dk_sum, es_old,
		es_new, es_rate, es_sum, vmtot, flip, flsp)
	struct ss_cfg *sc;
	struct vmmeter *vm_old;
	struct vmmeter *vm_new;
	struct vmmeter *vm_rate;
	struct dk *dk_old;
	struct dk *dk_new;
	struct dk *dk_rate;
	struct dk *dk_sum;
	struct ether_stat *es_old;
	struct ether_stat *es_new;
	struct ether_stat *es_rate;
	struct ether_stat *es_sum;
	struct vmtotal *vmtot;
	struct flckinfo *flip;
	struct flock_stats *flsp;
{
	extern struct timeval todop();
	register int i;
	register double etime;
	register unsigned *osp, *nsp, *rsp;

	/*
	 * Calculate time in the interval
	 * must divide by sc->nonline and HZ to get number of seconds.
	 */
	etime = 0.0;
	for (i = 0; i < CPUSTATES; i++)
		etime += vm_new->v_time[i] - vm_old->v_time[i];
	etime /= (sc->nonline * HZ);
	if (etime == 0.0)
		etime = 1.0;

	/*
	 * Calculate the rate for vm stuff
	 */
	rsp = &(vm_rate->v_first);
	osp = &(vm_old->v_first);
	nsp = &(vm_new->v_first);
	for (; rsp <= &(vm_rate->v_last); rsp++, osp++, nsp++) {
		/* the "so" macro handles wrap-around */
		*rsp = (so(*nsp,*osp) / (double)etime) + 0.5;
	}
	/* Convert physical i/o blks to KB */
	vm_rate->v_phyr >>= 1;
	vm_rate->v_phyw >>= 1;
	if (vm_rate->v_lreads != 0 && 
	    vm_rate->v_lreads > vm_rate->v_rcount)
		fsreadhit = 100. * (vm_rate->v_lreads - vm_rate->v_rcount)
					/ vm_rate->v_lreads + 0.5;
	else
		fsreadhit = 0;
	if (vm_rate->v_lwrites != 0 &&
	    vm_rate->v_lwrites > vm_rate->v_wcount)
		fswritehit = 100. * (vm_rate->v_lwrites - vm_rate->v_wcount)
					/ vm_rate->v_lwrites + 0.5;
	else
		fswritehit = 0;
	/*
	 * Convert vmtotal memory stats and deficit, avedirty to 1K units
	 */
	vmtot->t_vm <<= (PGSHIFT - 10);
	vmtot->t_avm <<= (PGSHIFT - 10);
	vmtot->t_rm <<= (PGSHIFT - 10);
	vmtot->t_arm <<= (PGSHIFT - 10);
	vmtot->t_free <<= (PGSHIFT - 10);
	avedirty <<= (PGSHIFT - 10);
	deficit <<= (PGSHIFT - 10);
	/*
	 * Calculate rate and sum for disk stuff
	 * dk_blks is converted to KB's here (assumes 512 byte blks)
	 */
	bzero((char *)dk_sum, sizeof(struct dk));
	for (i = 0; i < sc->dk_nxdrive; i++) {
#ifdef NOTYET
		dk_rate[i].dk_time = todop('-', dk_new[i].dk_time,
						dk_old[i].dk_time);
#endif
		dk_rate[i].dk_seek =
			(dk_new[i].dk_seek - dk_old[i].dk_seek) / etime + 0.5;
		dk_rate[i].dk_xfer =
			(dk_new[i].dk_xfer - dk_old[i].dk_xfer) / etime + 0.5;
		dk_rate[i].dk_blks =
			(dk_new[i].dk_blks - dk_old[i].dk_blks) / (2*etime)+0.5;

#ifdef NOTYET
		dk_sum->dk_time = todop('+', dk_sum->dk_time, dk_rate[i].dk_time);
		bcopy(dk_new[i].dk_name, dk_sum->dk_name, 10);
#endif
		dk_sum->dk_seek += dk_rate[i].dk_seek;
		dk_sum->dk_xfer += dk_rate[i].dk_xfer;
		dk_sum->dk_blks += dk_rate[i].dk_blks;
	}
	/*
	 * Calculate rate for ether stuff
	 */
	es_sum->pktin = 0;
	es_sum->pktout = 0;
	for (i = 0; i < sc->nse_unit; i++) {
		es_rate[i].pktin = (es_new[i].pktin - es_old[i].pktin) / etime + 0.5;
		es_rate[i].pktout = (es_new[i].pktout - es_old[i].pktout) / etime + 0.5;
		es_sum->pktin += es_rate[i].pktin;
		es_sum->pktout += es_rate[i].pktout;
	}
	/*
	 * Calculate file locking stats
	 */
	if (flip->recs != 0)
		flsp->lck_ut = flip->reccnt * 100. / (double) flip->recs;
	else
		flsp->lck_ut = 0;

	if (flip->fils != 0)
		flsp->fil_ut = flip->filcnt * 100. / (double) flip->fils;
	else
		flsp->fil_ut = 0;
}

/*
 * Perform operations on struct timeval's
 */
struct timeval
todop(op, s1, s2)
	int op;
	struct timeval s1, s2; 
{
	struct timeval result;

	switch (op) {
	case '+':
		result.tv_sec = s1.tv_sec + s2.tv_sec;
		result.tv_usec = s1.tv_usec + s2.tv_usec;
		if (result.tv_usec >= 1000000) {
			result.tv_sec++;
			result.tv_usec -= 1000000;
		}
		break;
	case '-':
		result.tv_sec = s1.tv_sec - s2.tv_sec;
		if (s1.tv_usec >= s2.tv_usec) {
			result.tv_usec = s1.tv_usec - s2.tv_usec;
		} else {
			result.tv_sec--;
			result.tv_usec = 1000000 + s1.tv_usec - s2.tv_usec;
		}
		break;
	}
	return(result);
}

perprocrate(sc, old, new, procutil)
	struct ss_cfg *sc;
	register struct vmmeter *old;
	register struct vmmeter *new;
	unsigned *procutil;
{
	register int j;
	register unsigned etime;
	register unsigned *up;
	int i;

	for (i = 0; i < sc->Nengine; i++) {
		up = procutil;
		etime = 0;
		for (j = 0; j < CPUSTATES; j++, up++) {
			*up = new->v_time[j] - old->v_time[j];
			etime += *up;
		}
		up = procutil;
		if (etime != 0) {
			for (j = 0; j < CPUSTATES; j++, up++) {
				*up = (*up / (double)etime) * 100. +0.5;
			}
		}
		new++;
		old++;
		procutil += CPUSTATES;
	}
}

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
static	char	rcsid[] = "$Header: kp_nmi.c 1.5 87/09/24 $";
#endif

/* $Log:	kp_nmi.c,v $
 */

/*
 * kp_nmi.c - kernel profiler NMI handler
 */

#include "../h/param.h"
#include "../h/vm.h"
#include "../h/mutex.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/uio.h"
#include "../h/systm.h"
#include "../h/ioctl.h"

#include "../balance/slic.h"
#include "../balance/cfg.h"

#include "../machine/ioconf.h"
#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/hwparam.h"
#include "../machine/plocal.h"
#include "../machine/intctl.h"
#include "../machine/mftpr.h"
#include "../machine/exec.h"

#include "../sec/sec.h"		/* scsi common data structures */
#include "../sec/kp.h"		/* driver local structures */

/*
 * Global data structures.
 */
extern u_long *kp_nmis;		/* ptr to # of NMI's recv'd per processor */
extern struct pc_mode *kp_pc_m;	/* ptr to pc/mode pairs */
extern int *kp_in_nmi;		/* "Currently handling nmi" flags */
extern int *kp_ov_nmi;		/* Overlapped NMI count */


/*
 * kp_nmi(): Handle profiler NMI's
 *		returns:	0 if profiler NMI
 *				1 otherwise
 */

/*ARGSUSED*/
kp_nmi(type, edi, esi, ebp, unused, ebx, edx, ecx, eax, eip, cs, flags, sp, ss)
	int	type;		/* type of trap */
	unsigned ss;		/* only if inter-segment (user->kernel) */
	unsigned sp;		/* only if inter-segment (user->kernel) */
	unsigned flags;		/* extended-flags (see machine/psl.h) */
	unsigned cs;		/* sense user-mode entry from RPL field */
	unsigned eip;		/* return context */
	unsigned eax;		/* scratch registers */
	unsigned ecx;		/* ditto */
	unsigned edx;		/* more such */
	unsigned ebx;		/* register variable */
	unsigned unused;	/* temp SP (from push-all instruction) */
	unsigned ebp;		/* of interrupted frame */
	unsigned esi;		/* register variable */
	unsigned edi;		/* register variable */
{
	register int me = l.me;

	if (va_slic->sl_nmiint == NMI_PROF) {
		/*
		 * Check for overlapped nmi's and just return if detected
		 */
		if (++kp_in_nmi[me] > 1) {
			--kp_in_nmi[me];
			++kp_ov_nmi[me];
			return(0);
		}
		/*
		 * store away the pc and mode for the SCED
		 */
		kp_pc_m[me].pm_pc = eip;
		kp_pc_m[me].pm_mode = USERMODE(cs);
		/* instrumentation */
		kp_nmis[me]++;
		--kp_in_nmi[me];
		return(0);
	} else {
		return(1);
	}
}

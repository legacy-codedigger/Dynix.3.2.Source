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
 * $Header: gate.h 2.13 89/06/26 $
 *
 * gate.h
 *	Gate definitions and allocations.  i386 version.
 *
 * SLIC gates are not used in the i386 kernel, rather bytes of memory
 * serve as spin locks.  Most gates in the kernel are used to construct
 * locks and sema's; thus use of these gates is invisible and can be
 * ignored in the i386 kernel (init_sema() and init_lock() are macros
 * that ignore gate numbers).
 *
 * Several of the gates are used directly.  These are represented
 * by individual bytes of memory (see machine/gate_data.c).
 *
 * K20 kernel hashes address of byte to determine SLIC gate to use.
 */

/* $Log:	gate.h,v $
 */

/*
 * Typedef for padded gates, arranges that the gates are in different
 * cache blocks.
 */

typedef	char	pgate_t[CACHESIZE];

/*
 * Explicitly referenced gates.  Set is subject to change.
 */

extern	pgate_t		g_time;			/* time and timeout queue */
#define	G_TIME		(*(gate_t *)g_time)

extern	pgate_t		g_cfree;		/* cfreelist gating */
#define	G_CFREE		(*(gate_t *)g_cfree)

extern	pgate_t		g_runq;			/* dispatcher run-queues */
#define	G_RUNQ		(*(gate_t *)g_runq)

extern	pgate_t		g_swap;			/* swapmap (vm_sw.c) */
#define	G_SWAP		(*(gate_t *)g_swap)

extern	pgate_t		g_netisr;		/* network software intr's */
#define	G_NETISR	(*(gate_t *)g_netisr)

/*
 * G_FS (used in FSLOCK(), FSUNLOCK()) should be distributed among the
 * file-systems (ie, use a per file-sys gate, or atomic inc/dec).
 */

extern	pgate_t		g_fs;			/* file-system locking */
#define	G_FS		(*(gate_t *)g_fs)

/*
 * These gates are used to init locks and semas thru interfaces other
 * than init_lock(), init_sema(), or via expressions.  Thus must define
 * symbols for code compatibiity.  These symbols are not used otherwise.
 */

#define	G_SWMIN	0
#define	G_SWMAX	0
#define	G_INOMIN 0
#define	G_INOMAX 0
#define	G_PTY	0
#ifdef QUOTA
#define G_QUOTA	0
#endif

/*
 * Macro to declare spl_t variable that only holds return value from p_gate.
 * On SGS, this is null -- ie, no need.
 */

#define	GATESPL(s)

#if	defined(KXX) || defined(SLIC_GATES)
/*
 * GATE_GROUP is the "destination" (eg, group) used for gate accesses.
 * Currently defined to be everybody.
 */

#define	GATE_GROUP	(SL_GROUP|SL_ALL)
#endif	KXX

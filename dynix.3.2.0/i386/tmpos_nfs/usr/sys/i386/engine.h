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
 * $Header: engine.h 2.9 90/11/08 $
 *
 * engine.h
 *	Per-processor basic "engine" structure.  Fundamental representation
 *	of a processor for dispatching and initialization.
 *
 * Allocated per-processor at boot-time in an array.  Base stored in
 * "engine".
 */

/* $Log:	engine.h,v $
 */

struct	engine	{
	struct proc	*e_head;	/* head of processor run-queue	*/
	struct proc	*e_tail;	/* tail of processor run-queue	*/
	u_char		e_slicaddr;	/* the processor's SLIC address	*/
	u_int		e_diag_flag;	/* copy of power-up diagnostic flags */
	short		e_flags;	/* processor flags - see below	*/
	short		e_state;	/* BOUND or GLOBAL for affinity	*/
	char		e_pri;		/* priority of current process	*/
	char		e_npri;		/* nudged priority		*/
	int		e_count;	/* number processes bound	*/
	int		e_cpu_speed;	/* copy of config cpu speed     */
	struct ppriv_pages *e_local;	/* physical address of local stuff */
};

/* currently defined flag bits */
#define	E_OFFLINE	0x01		/* processor is off-line	*/
#define E_BAD		0x02		/* processor is bad		*/
#define	E_SHUTDOWN	0x04		/* shutdown has been requested	*/ 
#define E_DRIVER	0x08		/* processor has driver bound	*/
#define E_PAUSED	0x10		/* processor paused - see panic */
#define	E_FPU387	0x20		/* 1==387 0 not (i386 only) */
#define	E_FPA		0x40		/* processor has an FPA (i386 only) */
#define E_SGS2          0x80            /* processor is SGS2 (i.e. scan based)*/
#define E_NOWAY		(E_OFFLINE|E_BAD|E_SHUTDOWN|E_PAUSED)

/* defined for state field */
#define	E_BOUND		0x01		/* processor is running bound process */
#define	E_GLOBAL	0x00		/* processor not running bound process*/

/* Cannot switch process to Engine - see runme */
#define E_UNAVAIL	-1

#ifdef KERNEL
extern	lock_t		engtbl_lck;	/* lock for count fields */
extern	sema_t		eng_wait;	/* coordinate on/off with tmp_ctl */
extern	struct engine	*engine;	/* Engine Array Base */
extern	struct engine	*engine_Nengine;/* just past Engine Array Base */
extern	unsigned 	Nengine;	/* # Engines to alloc at boot */
extern	unsigned	nonline;	/* count of online engines */
#endif KERNEL

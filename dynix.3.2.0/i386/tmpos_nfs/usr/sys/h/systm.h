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
 * $Header: systm.h 2.7 87/09/15 $
 *
 * systm.h
 *	Random set of variables used by more than one routine.
 */

/* $Log:	systm.h,v $
 */

extern	char version[];		/* system version */

/*
 * Nblkdev is the number of entries
 * (rows) in the block switch. It is
 * set in conf.c by sizing the switch.
 * Used in bounds checking on major
 * device numbers.
 */
extern	int	nblkdev;

/*
 * Number of character switch entries.
 * Set in conf.c by sizing the switch.
 */
extern	int	nchrdev;

extern	int	nswdev;		/* number of swap devices */
extern	int	mpid;		/* generic for unique process id's */

extern	sema_t	runout;		/* swap scheduling synchronization */

extern	int	maxmem;		/* actual max memory per process */

extern	int	nswap;		/* size of swap space */
extern	dev_t	rootdev;	/* device of the root */
extern	dev_t	swapdev;	/* swapping device */
extern	dev_t	argdev;		/* argument list device */
extern	struct	vnode	*swapdev_vp;	/* vnode equivalent of swapdev */
extern	struct	vnode	*argdev_vp;	/* vnode equivalent of argdev */

extern	char	icode[];	/* user init code */
extern	int	szicode;	/* its size */

daddr_t	bmap();
caddr_t	calloc();
unsigned max();
unsigned min();
int	memall();
int	vmemall();
caddr_t	wmemall();
#ifdef	NFS
caddr_t	kmem_alloc();
#endif	NFS

/*
 * Structure of the system-entry table
 */
extern struct sysent {
	int	sy_narg;	/* total number of arguments */
	int	(*sy_call)();	/* handler */
} sysent[];

extern	sema_t	selwait;
extern	lock_t	select_lck;

extern	sema_t	unmount_mutex;

extern	u_int	boothowto;	/* reboot flags, from power-up diagnostics */
extern	u_int	sys_clock_rate;	/* # Mega Hz system runs at */

extern struct panic_data {
	struct engine *pd_engine;	/* Panicing engine */
	struct proc *pd_proc;		/* Panicing process, 0 if idle */
	caddr_t	pd_sp;			/* sp to saved regs in panic frame */
	caddr_t pd_dblsp;		/* sp to saved regs in dblpanic frame */
} panic_data; 

extern	int	defssiz;	/* default stack limit */

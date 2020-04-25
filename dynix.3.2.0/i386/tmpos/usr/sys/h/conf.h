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
 * $Header: conf.h 2.0 86/01/28 $
 *
 * conf.h
 *	Configuration switch definitions.
 */

/* $Log:	conf.h,v $
 */

/*
 * Declaration of block device
 * switch. Each entry (row) is
 * the only link between the
 * main unix code and the driver.
 * The initialization of the
 * device switches is in the
 * file conf.c.
 *
 * Dumping is handled by non-kernel functions, hence d_dump is removed.
 */
struct bdevsw
{
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_strategy)();
	int	(*d_minphys)();
	int	(*d_psize)();
	int	d_flags;
};
#ifdef	KERNEL
extern	struct	bdevsw bdevsw[];
#endif	KERNEL

/*
 * Character device switch.
 * d_reset is removed in this implementation.
 */
struct cdevsw
{
	int	(*d_open)();
	int	(*d_close)();
	int	(*d_read)();
	int	(*d_write)();
	int	(*d_ioctl)();
	int	(*d_stop)();
	int	(*d_select)();
	int	(*d_mmap)();
};
#ifdef	KERNEL
extern	struct	cdevsw cdevsw[];
#endif	KERNEL

/*
 * tty line control switch.
 * l_rend, l_meta, l_modem are dropped and l_select is added
 * in this implementation.
 */
struct linesw
{
	int	(*l_open)();
	int	(*l_close)();
	int	(*l_read)();
	int	(*l_write)();
	int	(*l_ioctl)();
	int	(*l_select)();
	int	(*l_rint)();
	int	(*l_start)();
};
#ifdef	KERNEL
extern	struct	linesw linesw[];
#endif	KERNEL

/*
 * Swap device information
 */
struct swdevt
{
	dev_t	sw_dev;
	int	sw_freed;
	int	sw_nblks;
};
#ifdef	KERNEL
extern	struct	swdevt swdevt[];
#endif	KERNEL

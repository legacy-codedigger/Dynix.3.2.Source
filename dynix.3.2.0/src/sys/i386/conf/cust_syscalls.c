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
static	char	rcsid[] = "$Header: cust_syscalls.c 2.9 88/03/09 $";
#endif

/*
 * cust_syscalls.c
 *	Custom system-call hooks.
 *
 * This file contains stubs for customer system calls.  This file can be
 * edited to implement custom system calls.  However, no entry points
 * in this file can be removed.  Stubs cust[0-6]  have 0 through 6 
 * arguments, respectively.  Stubs cust[7-9]  have one argument each.  
 */

/* $Log:	cust_syscalls.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/kernel.h"

#include "../balance/slic.h"

/*
 * cust_sys_boot()
 *	Allocate required data structures.
 *
 * Called just after devices are configured to allow data-structure
 * allocation and initialization for custom system calls.  Most
 * kernel data structures are sized (exception: nbuf, nswbuf, # pages of
 * cache buffers, cmap[] array, kernel page table).
 *
 * Up on one processor, and no processes exist yet; thus don't
 * use any blocking primitive (eg, p_sema()).  Printf() is ok.
 */

cust_sys_boot()
{
}

/*
 * cust_sys_fork()
 *	Process is forking.
 *
 * Called during fork and vfork to give custom system calls a chance
 * to update resource counters, statistics, etc.
 *
 * Called just after file-descriptors are dup'd into the child process,
 * but before it's address space fully exists.
 *
 * u.* is the parent process.
 */

cust_sys_fork(child, isvfork)
	struct	proc	*child;
	int		isvfork;
{
#ifdef	lint
	if (isvfork)
		child->p_link = child;
#endif	lint
}

/*
 * cust_sys_exec()
 *	Process is successfully performing an execve().
 *
 * Called during (successful) exec system call (all flavors) to give
 * custom system calls a chance to update resource counters, statistics, etc.
 *
 * Called just before file-descriptors are scanned for "close on exec".
 * Thus old process image is gone; running on new process image.
 *
 * u.* is the parent process.
 */

cust_sys_exec()
{
}

/*
 * cust_sys_exit()
 *	Process is exiting.
 *
 * Called during exit system call to give custom system calls a chance to
 * update resource counters, statistics, etc.
 *
 * Called when process image still exists and before file-descriptors
 * are closed.
 *
 * Calling process is not swappable and is ignoring all signals.
 *
 * u.* is the parent process.
 */

cust_sys_exit()
{
}

/*
 * The remaining procedures are custom system call implementations.
 */

cust0()
{
	nosys();
}



cust1()
{
	register struct a {
		int	arg1;
	} *uap = (struct a *)u.u_ap;
#ifdef	lint
	uap->arg1 = 0;
#endif	lint
	nosys();
	
}

cust2()
{
	register struct a {
		int	arg1;
		int	arg2;
	} *uap = (struct a *)u.u_ap;

#ifdef	lint
	uap->arg1 = 0;
	uap->arg2 = 0;
#endif	lint
	nosys();
}

cust3()
{
	register struct a {
		int	arg1;
		int	arg2;
		int	arg3;
	} *uap = (struct a *)u.u_ap;

#ifdef	lint
	uap->arg1 = 0;
	uap->arg2 = 0;
	uap->arg3 = 0;
#endif	lint
	nosys();
}

cust4()
{
	register struct a {
		int	arg1;
		int	arg2;
		int	arg3;
		int	arg4;
	} *uap = (struct a *)u.u_ap;

#ifdef	lint
	uap->arg1 = 0;
	uap->arg2 = 0;
	uap->arg3 = 0;
	uap->arg4 = 0;
#endif	lint
	nosys();
}

cust5()
{
	register struct a {
		int	arg1;
		int	arg2;
		int	arg3;
		int	arg4;
		int	arg5;
	} *uap = (struct a *)u.u_ap;

#ifdef	lint
	uap->arg1 = 0;
	uap->arg2 = 0;
	uap->arg3 = 0;
	uap->arg4 = 0;
	uap->arg5 = 0;
#endif	lint
	nosys();
}

cust6()
{
	register struct a {
		int	arg1;
		int	arg2;
		int	arg3;
		int	arg4;
		int	arg5;
		int	arg6;
	} *uap = (struct a *)u.u_ap;

#ifdef	lint
	uap->arg1 = 0;
	uap->arg2 = 0;
	uap->arg3 = 0;
	uap->arg4 = 0;
	uap->arg5 = 0;
	uap->arg6 = 0;
#endif	lint
	nosys();
}

cust7()
{
	register struct a {
		int	arg1;
	} *uap = (struct a *)u.u_ap;

#ifdef	lint
	uap->arg1 = 0;
#endif	lint
	nosys();
}

cust8()
{
	register struct a {
		int	arg1;
	} *uap = (struct a *)u.u_ap;

#ifdef	lint
	uap->arg1 = 0;
#endif	lint
	nosys();
}

cust9()
{
	register struct a {
		int	arg1;
	} *uap = (struct a *)u.u_ap;

#ifdef	lint
	uap->arg1 = 0;
#endif	lint
	nosys();
}



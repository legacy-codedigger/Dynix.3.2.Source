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
static	char	rcsid[] = "$Header: kern_proc.c 2.3 86/09/29 $";
#endif

/* $Log:	kern_proc.c,v $
 */

#include "../h/param.h"
#include "../h/mutex.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/user.h"
#include "../h/kernel.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/acct.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/file.h"
#include "../h/uio.h"
#include "../h/mbuf.h"

#include "../machine/reg.h"
#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/intctl.h"

/*
 * inferior()
 *	Is p an inferior of the current process?
 *
 * The proc_list lock is locked by the caller at SPL6 to guarantee
 * the sanity of the p-c-s chain.
 */

inferior(p)
	register struct proc *p;
{

	for (; p != u.u_procp; p = p->p_pptr)
		if (p->p_ppid == 0)
			return (0);
	return (1);
}

/*
 * pfind()
 *	Find process via hash list.
 *
 * If process found return with process state (p_state) locked.
 * Otherwise return 0.
 *
 * Assumes caller knows the appropriate ipl to v_lock with.
 */

struct proc *
pfind(pid)
	int pid;
{
	register struct proc *p;
	spl_t s_ipl;

	/*
	 * Must lock proc_list to keep pidhash list consistent.
	 */
	s_ipl = p_lock(&proc_list, SPL6);
	for (p = &proc[pidhash[PIDHASH(pid)]]; p != &proc[0]; p = &proc[p->p_idhash])
		if (p->p_pid == pid) {
			(void) p_lock(&p->p_state, SPLHI);
			v_lock(&proc_list, SPLHI);
			return (p);
		}
	v_lock(&proc_list, s_ipl);
	return((struct proc *)0);
}

/*
 * lpfind()
 *	locked pfind.  Find process via hash list.
 *
 * If process found return with process state (p_state) locked.
 * Otherwise return 0.
 *
 * Assumes proc_list is locked at SPL6 and caller knows which ipl
 * to v_lock with.
 */

struct proc *
lpfind(pid)
	int pid;
{
	register struct proc *p;

	for (p = &proc[pidhash[PIDHASH(pid)]]; p != &proc[0]; p = &proc[p->p_idhash])
		if (p->p_pid == pid) {
			(void) p_lock(&p->p_state, SPLHI);
			return (p);
		}
	return((struct proc *)0);
}

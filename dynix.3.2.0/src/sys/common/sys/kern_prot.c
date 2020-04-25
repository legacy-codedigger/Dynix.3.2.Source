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
static	char	rcsid[] = "$Header: kern_prot.c 2.6 89/08/11 $";
#endif

/*
 * kern_prot.c
 *	System calls related to processes and protection
 */

/* $Log:	kern_prot.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/acct.h"

#include "../machine/intctl.h"
#include "../machine/gate.h"

void	crfree();

getpid()
{
	u.u_r.r_val1 = u.u_procp->p_pid;
	u.u_r.r_val2 = u.u_procp->p_ppid;
}

getpgrp()
{
	register struct a {
		int	pid;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	/*
	 * If process is the current process, get pgrp
	 * and return.
	 */
	if (uap->pid == 0) {
		u.u_r.r_val1 = u.u_procp->p_pgrp;
		return;
	}
	/* if process found, pfind locks its p_state */
	p = pfind(uap->pid);
	if (p == 0) {
		u.u_error = ESRCH;
		return;
	}
	u.u_r.r_val1 = p->p_pgrp;
	/* free lock and back to base level */
	v_lock(&p->p_state, SPL0);
}

getuid()
{
	u.u_r.r_val1 = u.u_ruid;
	u.u_r.r_val2 = u.u_uid;
}

getgid()
{
	u.u_r.r_val1 = u.u_rgid;
	u.u_r.r_val2 = u.u_gid;
}

getgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;

	for (gp = &u.u_groups[NGROUPS]; gp > u.u_groups; gp--)
		if (gp[-1] != NOGROUP)
			break;
	if (uap->gidsetsize < gp - u.u_groups) {
		u.u_error = EINVAL;
		return;
	}
	uap->gidsetsize = gp - u.u_groups;
	u.u_error = copyout((caddr_t)u.u_groups, (caddr_t)uap->gidset,
	    uap->gidsetsize * sizeof (u.u_groups[0]));
	if (u.u_error)
		return;
	u.u_r.r_val1 = uap->gidsetsize;
}

setpgrp()
{
	register struct proc *p;
	register struct a {
		int	pid;
		int	pgrp;
	} *uap = (struct a *)u.u_ap;
	spl_t s_ipl;

	if (uap->pid == 0) {
		u.u_procp->p_pgrp = uap->pgrp;
		return;
	}
	/*
	 * Must lock proc_list for inferior() and lpfind to maintain p-c-s
	 * integrity. lpfind() returns a locked p_state if process found.
	 * The proc_list must be locked before the p_state to maintain the
	 * ordering defined by exit. Otherwise, deadlock will occur.
	 */
	s_ipl = p_lock(&proc_list, SPL6);
	p = lpfind(uap->pid);
	if (p == 0) {
		v_lock(&proc_list, s_ipl);
		u.u_error = ESRCH;
		return;
	}
/* need better control mechanisms for process groups */
	if (p->p_uid != u.u_uid && u.u_uid && !inferior(p)) {
		v_lock(&p->p_state, SPL6);
		v_lock(&proc_list, s_ipl);
		u.u_error = EPERM;
		return;
	}
	p->p_pgrp = uap->pgrp;
	v_lock(&p->p_state, SPL6);
	v_lock(&proc_list, s_ipl);
}

setreuid()
{
	struct a {
		int	ruid;
		int	euid;
	} *uap;
	register int ruid, euid;

	uap = (struct a *)u.u_ap;
	ruid = uap->ruid;
	if (ruid == -1)
		ruid = u.u_ruid;
	if (u.u_ruid != ruid && u.u_uid != ruid && !suser())
		return;
	euid = uap->euid;
	if (euid == -1)
		euid = u.u_uid;
	if (u.u_ruid != euid && u.u_uid != euid && !suser())
		return;
	/*
	 * Everything's okay, do it.
	 */

	u.u_cred = crcopy(u.u_cred);
	u.u_procp->p_uid = ruid;
	u.u_ruid = ruid;
	u.u_uid = euid;
}

setregid()
{
	register struct a {
		int	rgid;
		int	egid;
	} *uap;
	register int rgid, egid;

	uap = (struct a *)u.u_ap;
	rgid = uap->rgid;
	if (rgid == -1)
		rgid = u.u_rgid;
	if (u.u_rgid != rgid && u.u_gid != rgid && !suser())
		return;
	egid = uap->egid;
	if (egid == -1)
		egid = u.u_gid;
	if (u.u_rgid != egid && u.u_gid != egid && !suser())
		return;
	u.u_cred = crcopy(u.u_cred);
	if (u.u_rgid != rgid) {
		leavegroup(u.u_rgid);
		(void) entergroup(rgid);
		u.u_rgid = rgid;
	}
	u.u_gid = egid;
}

setgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;
	struct ucred *newcr, *tmpcr;

	if (!suser())
		return;
	if (uap->gidsetsize > sizeof (u.u_groups) / sizeof (u.u_groups[0])) {
		u.u_error = EINVAL;
		return;
	}
	newcr = crdup(u.u_cred);
	u.u_error = copyin((caddr_t)uap->gidset, (caddr_t)newcr->cr_groups,
	    uap->gidsetsize * sizeof (newcr->cr_groups[0]));
	if (u.u_error) {
		crfree(newcr);
		return;
	}
	tmpcr = u.u_cred;
	u.u_cred = newcr;
	crfree(tmpcr);
	for (gp = &u.u_groups[uap->gidsetsize]; gp < &u.u_groups[NGROUPS]; gp++)
		*gp = NOGROUP;
}

/*
 * Group utility functions.
 */

/*
 * Delete gid from the group set.
 */
leavegroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			goto found;
	return;
found:
	for (; gp < &u.u_groups[NGROUPS-1]; gp++)
		*gp = *(gp+1);
	*gp = NOGROUP;
}

/*
 * Add gid to the group set.
 */
entergroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			return (0);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp < 0) {
			*gp = gid;
			return (0);
		}
	return (-1);
}

/*
 * Check if gid is a member of the group set.
 */
groupmember(gid)
	int gid;
{
	register int *gp;

	if (u.u_gid == gid)
		return (1);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
		if (*gp == gid)
			return (1);
	return (0);
}

/*
 * Routines to allocate and free credentials structures
 * These could all be macros.
 *
 * Free-list is kept thru cr_groups[0].  This is kindof a kludge, but
 * we avoid the "union" since the free-list is kept local to this module.
 */

/*
 * ucred_init()
 *	Initialize credentials list, locks, and semaphores.
 *
 * Assumes boot-time code allocated cred data-structures.
 * Assumes caller of crget() init's cr_lock.
 */

int		nucred;				/* # cred's allocated */
struct	ucred	*ucred_free;			/* head of free-list */

lock_t		ucred_mutex;			/* for cred-list manipulation */
sema_t		ucred_wait;			/* wait here for free cred */

#define		CR_NEXT(cr)	(*(struct ucred **)&((cr)->cr_groups[0]))

ucred_init()
{
	register struct	ucred	*cr;
	register int i;

	init_lock(&ucred_mutex, G_CRED);
	init_sema(&ucred_wait, 0, 0, G_CRED);

	for (cr = ucred_free, i = 0; i < nucred; i++, cr++)
		CR_NEXT(cr) = (cr+1);
	CR_NEXT(cr-1) = NULL;
}

/*
 * crhold()
 *	Hold a ucred structure (add a reference).
 */

void
crhold(cr)
	struct ucred *cr;
{
	spl_t	s;

	s = p_lock(&cr->cr_lock, SPLHI);
	cr->cr_ref++;
	v_lock(&cr->cr_lock, s);
}

/*
 * crget()
 *	Allocate a ucred structure.
 *
 * The structure is not filled out or zeroed; caller must do this.
 *
 * The Caller must init the lock and ref count,
 * because these may be incorrect (due to concurrent activity)
 * if the caller copies another ucred struct into the new struct.
 */

struct ucred *
crget()
{
	struct ucred *cr;
	spl_t	s;

	s = p_lock(&ucred_mutex, SPLIMP);

	while (ucred_free == NULL) {
		p_sema_v_lock(&ucred_wait, PRSWAIT, &ucred_mutex, s);
		s = p_lock(&ucred_mutex, SPLIMP);
	}

	cr = ucred_free;
	ucred_free = CR_NEXT(cr);

	v_lock(&ucred_mutex, s);

	return(cr);
}

/*
 * crfree()
 *	Free a ucred structure.
 *
 * Releases ucred structure to free-list when ref count gets to 0.
 */

void
crfree(cr)
	register struct ucred *cr;
{
	spl_t	s;

	s = p_lock(&cr->cr_lock, SPLHI);
	if (--cr->cr_ref)
		v_lock(&cr->cr_lock, s);
	else {
		/*
		 * Ref-cnt == 0 so we are last holder of ucred and can free it.
		 */
		(void) p_lock(&ucred_mutex, SPLHI);
		CR_NEXT(cr) = ucred_free;
		ucred_free = cr;
		if (blocked_sema(&ucred_wait))
			v_sema(&ucred_wait);
		v_lock(&ucred_mutex, s);
	}
}

/*
 * crcopy()
 *	Copy ucred structure to a new one and free the old one.
 */

struct ucred *
crcopy(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	/*
	 * Mutex: cr data is safe since caller has crhold() on it.
	 * Lock and count may be changing, and must be re-initialized.
	 */

	newcr = crget();
	*newcr = *cr;					/* Struct copy */
	newcr->cr_ref = 1;
	init_lock(&newcr->cr_lock, G_CRED);
	crfree(cr);

	return(newcr);
}

/*
 * crdup()
 *	Dup ucred struct to a new held one.
 */

struct ucred *
crdup(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	/*
	 * Mutex: cr data is safe since caller has crhold() on it.
	 * Lock and count may be changing, and must be re-initialized.
	 */

	newcr = crget();
	*newcr = *cr;					/* Struct Copy */
	newcr->cr_ref = 1;
	init_lock(&newcr->cr_lock, G_CRED);

	return(newcr);
}

/*
 * suser()
 *	Test if the current user is the super user.
 */

suser()
{
	if (u.u_uid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	u.u_error = EPERM;
	return (0);
}

/*
 * System 5 setuid
 */
att_setuid()
{
	register struct a {
		int	uid;
	} *uap = (struct a *)u.u_ap;
	register int uid;
	register struct proc *p = u.u_procp;

	uid = uap->uid;
	if ((uid >= MAXUID) || (uid < 0)) {
		u.u_error = EINVAL;
		return;
	}
	if (u.u_uid && (uid == u.u_ruid || uid == p->p_suid)) {
		u.u_cred = crcopy(u.u_cred);
		u.u_uid = uid;
	} else if (suser()) {
		u.u_cred = crcopy(u.u_cred);
		u.u_uid = uid;
		p->p_uid = uid;
		p->p_suid = uid;
		u.u_ruid = uid;
	}
}

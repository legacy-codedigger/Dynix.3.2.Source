/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
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
static	char	rcsid[] = "$Header: sys_vm.c 2.4 1991/05/30 00:01:12 $";
#endif

/*
 * sys_vm.c
 *	VM Related system-calls.
 */

/* $Log: sys_vm.c,v $
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"

#include "../machine/intctl.h"

vm_ctl()
{
	register struct a {
		int	funct;
		caddr_t	stuff;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p = u.u_procp;
	register long	wantRS;
	register spl_t	s;
	struct	vm_tune	vm_arg;
	extern	bool_t	root_vm_setrs;
	extern	bool_t	root_vm_swapable;
	extern	bool_t	root_vm_pffable;

	switch(uap->funct) {

	case VM_GETPARAM:
		u.u_error = copyout((caddr_t)&vmtune, uap->stuff,
							sizeof(struct vm_tune));
		break;

	case VM_SETPARAM:
		u.u_error = copyin(uap->stuff, (caddr_t)&vm_arg,
							sizeof(struct vm_tune));
		if (u.u_error || !suser())
			break;
		
		/*
		 * Some basic sanity checks on the data.
		 * These insure no kernel panics and basic sanity; it
		 * possible to completely mess up the system with
		 * inappropriate combinations of values here.
		 *
		 * Insisting minRS,maxRS >= minRS insures a RAW IO can
		 * make progress.
		 *
		 * minRS is coerced <= maxforkRS to insure forks can
		 * proceed.
		 */

#define	BOUND(a,op,b) if (!(a op b)) a = b

		BOUND(vm_arg.vt_maxRS, <=, maxRS);
		BOUND(vm_arg.vt_maxRS, >=, minRS);

		BOUND(vm_arg.vt_minRS, <=, vm_arg.vt_maxRS);
		BOUND(vm_arg.vt_minRS, <=, maxforkRS);
		BOUND(vm_arg.vt_minRS, >=, minRS);

		BOUND(vm_arg.vt_RSexecmult, >=, 1);
		BOUND(vm_arg.vt_RSexecdiv, >=, 1);

		BOUND(vm_arg.vt_PFFvtime, >=, 0);
		BOUND(vm_arg.vt_PFFhigh, >=, 0);
		BOUND(vm_arg.vt_PFFlow, >=, 0);
		BOUND(vm_arg.vt_PFFlow, <=, vm_arg.vt_PFFhigh);
		BOUND(vm_arg.vt_PFFincr, >=, 0);
		BOUND(vm_arg.vt_PFFdecr, >=, 0);

		/* not very scientific but will catch goofs */
		BOUND(vm_arg.vt_desfree, <=, (((maxRS - minRS)*9)/10));
		BOUND(vm_arg.vt_desfree, >=, (minRS+1) );
		BOUND(vm_arg.vt_minfree, <=, (((maxRS - minRS)*8)/10));
		BOUND(vm_arg.vt_minfree, <=, vm_arg.vt_desfree);
		BOUND(vm_arg.vt_minfree, >=, 1);
		/*
		 * All is at least sane; assign the real values...
		 */

		vmtune = vm_arg;
		break;

	case VM_GETRS:
		if (suword(uap->stuff, (int)p->p_rscurr) < 0)
			u.u_error = EFAULT;
		break;

	case VM_SETRS:
		/*
		 * Get and verify legality of new Rset size.
		 */

		if ((wantRS = fuword(uap->stuff)) == -1) {
			u.u_error = EFAULT;
			break;
		}

		if (wantRS < vmtune.vt_minRS
		||  wantRS > vmtune.vt_maxRS
		||  wantRS > p->p_maxrss) {
			u.u_error = EINVAL;
			break;
		}

		/*
		 * Is legal size; check for root and adjust self Rset size.
		 * vsetRS() can set a different value if this races with
		 * a concurrent vm_ctl(VM_SETPARAM), but don't worry about it.
		 */

		if (!root_vm_setrs || suser())
			vsetRS(wantRS);
		break;

	case VM_SWAPABLE:
		/*
		 * Declare self swap-able or not.
		 * If making self non-swap-able, might just need to be root...
		 */

		s = p_lock(&p->p_state, SPLHI);

		if (uap->stuff)				/* want swappable */
			p->p_flag &= ~SNOSWAP;
		else if (!root_vm_swapable || suser())	/* non-swappable */
			p->p_flag |= SNOSWAP;

		v_lock(&p->p_state, s);
		break;

	case VM_PFFABLE:
		/*
		 * Declare self PFF-able or not.
		 * If making self non-PFF-able, might just need to be root...
		 */

		s = p_lock(&p->p_state, SPLHI);

		if (uap->stuff)				/* want PFF-able */
			p->p_flag &= ~SNOPFF;
		else if (!root_vm_pffable || suser())	/* No PFF adjustment */
			p->p_flag |= SNOPFF;

		v_lock(&p->p_state, s);
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
}

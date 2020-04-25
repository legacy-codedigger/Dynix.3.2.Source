/* $Copyright:	$
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
static	char	rcsid[] = "$Header: shadow.c 1.1 91/01/21 $";
#endif

/*
 * shadow.c--support routines for managing shadow data structures across
 * major process events.
 *
 */

/* $Log:	shadow.c,v $
 */

#include <../../common/h/types.h>
#include <../../common/h/time.h>
#include <../../common/h/param.h>
#include <../../common/h/signal.h>
#include <../../common/h/proc.h>
#include <../../common/h/user.h>

extern caddr_t kmem_alloc();

/*
 * A process is fork()ing--duplicate resources appropriately.  Both processes
 * may be assumed to be swapped in and locked.
 */
shadow_fork(old, new)
	struct proc *old, *new;
{
	/* Duplicate FPA emulation save area if present */
	if (old->p_uarea->u_fpaesave) {
		new->p_uarea->u_fpaesave =
			(struct fpaesave *)kmem_alloc(sizeof(struct fpaesave));
		bcopy((caddr_t)old->p_uarea->u_fpaesave,
			(caddr_t)new->p_uarea->u_fpaesave,
			sizeof(struct fpaesave));
	}
}

/*
 * A process is exit()ing--free resources
 */
shadow_exit(p)
	struct proc *p;
{
	/* Free FPA emulation save area if present */
	if (p->p_uarea->u_fpaesave) {
		kmem_free((caddr_t)p->p_uarea->u_fpaesave,
			sizeof(struct fpaesave));
		p->p_uarea->u_fpaesave = NULL;
	}
}

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
static char rcsid[] = "$Header: pmap_prot.c 1.1 86/12/12 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)pmap_prot.c 1.1 85/05/30 Copyr 1984 Sun Micro";
#endif

/*
 * pmap_prot.c
 * Protocol for the local binder service, or pmap.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/pmap_prot.h>

#define NULL ((struct pmaplist *)0)

bool_t
xdr_pmap(xdrs, regs)
	XDR *xdrs;
	struct pmap *regs;
{

	if (xdr_u_long(xdrs, &regs->pm_prog) && 
		xdr_u_long(xdrs, &regs->pm_vers) && 
		xdr_u_long(xdrs, &regs->pm_prot))
		return (xdr_u_long(xdrs, &regs->pm_port));
	return (FALSE);
}

/* 
 * What is going on with linked lists? (!)
 * First recall the link list declaration from pmap_prot.h:
 *
 * struct pmaplist {
 *	struct pmap pml_map;
 *	struct pmaplist *pml_map;
 * };
 *
 * Compare that declaration with a corresponding xdr declaration that 
 * is (a) pointer-less, and (b) recursive:
 *
 * typedef union switch (bool_t) {
 * 
 *	case TRUE: struct {
 *		struct pmap;
 * 		pmaplist_t foo;
 *	};
 *
 *	case FALSE: struct {};
 * } pmaplist_t;
 *
 * Notice that the xdr declaration has no nxt pointer while
 * the C declaration has no bool_t variable.  The bool_t can be
 * interpreted as ``more data follows me''; if FALSE then nothing
 * follows this bool_t; if TRUE then the bool_t is followed by
 * an actual struct pmap, and then (recursively) by the 
 * xdr union, pamplist_t.  
 *
 * This could be implemented via the xdr_union primitive, though this
 * would cause a one recursive call per element in the list.  Rather than do
 * that we can ``unwind'' the recursion
 * into a while loop and do the union arms in-place.
 *
 * The head of the list is what the C programmer wishes to past around
 * the net, yet is the data that the pointer points to which is interesting;
 * this sounds like a job for xdr_reference!
 */
bool_t
xdr_pmaplist(xdrs, rp)
	register XDR *xdrs;
	register struct pmaplist **rp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	bool_t more_elements;
	register int freeing = (xdrs->x_op == XDR_FREE);
	register struct pmaplist **next;

	while (TRUE) {
		more_elements = (bool_t)(*rp != NULL);
		if (! xdr_bool(xdrs, &more_elements))
			return (FALSE);
		if (! more_elements)
			return (TRUE);  /* we are done */
		/*
		 * the unfortunate side effect of non-recursion is that in
		 * the case of freeing we must remember the next object
		 * before we free the current object ...
		 */
		if (freeing)
			next = &((*rp)->pml_next); 
		if (! xdr_reference(xdrs, (caddr_t *)rp,
		    (u_int)sizeof(struct pmaplist), xdr_pmap))
			return (FALSE);
		rp = (freeing) ? next : &((*rp)->pml_next);
	}
}

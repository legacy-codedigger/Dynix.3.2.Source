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
static char rcsid[] = "$Header: xdr_reference.c 1.2 88/03/14 $";
#endif

#ifndef lint
static char sccsid[] = "@(#)xdr_reference.c 1.1 85/05/30 Copyr 1984 Sun Micro";
#endif

/*
 * xdr_reference.c, Generic XDR routines impelmentation.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * "pointers".  See xdr.h for more info on the interface to xdr.
 */

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <stdio.h>
#define LASTUNSIGNED	((u_int)0-1)

/*
 * XDR an indirect pointer
 * xdr_reference is for recursively translating a structure that is
 * referenced by a pointer inside the structure that is currently being
 * translated.  pp references a pointer to storage. If *pp is null
 * the  necessary storage is allocated.
 * size is the sizeof the referneced structure.
 * proc is the routine to handle the referenced structure.
 */
bool_t
xdr_reference(xdrs, pp, size, proc)
	register XDR *xdrs;
	caddr_t *pp;		/* the pointer to work on */
	u_int size;		/* size of the object pointed to */
	xdrproc_t proc;		/* xdr routine to handle the object */
{
	register caddr_t loc = *pp;
	register bool_t stat;

	if (loc == NULL)
		switch (xdrs->x_op) {
		case XDR_FREE:
			return (TRUE);

		case XDR_DECODE:
			*pp = loc = mem_alloc(size);
			if (loc == NULL) {
				fprintf(stderr,
				    "xdr_reference: out of memory\n");
				return (FALSE);
			}
			bzero(loc, (int)size);
			break;
	}

	stat = (*proc)(xdrs, loc, LASTUNSIGNED);

	if (xdrs->x_op == XDR_FREE) {
		mem_free(loc, size);
		*pp = NULL;
	}
	return (stat);
}

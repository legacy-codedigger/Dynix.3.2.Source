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
static	char	rcsid[] = "$Header: conf_mmap.c 2.0 87/04/06 $";
#endif

/*
 * conf_mmap.c
 *	Vnode type -> mmap operations map.
 */

/* $Log:	conf_mmap.c,v $
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mutex.h"
#include "../h/user.h"
#include "../h/vnode.h"

/*
 * vnode_mmap[] is indexed by the vnode type to determine mapping operations.
 * Thus order must exactly match enum vtype (h/vnode.h).
 */

extern	struct	mapops	mmap_reg;
extern	struct	mapops	mmap_chr;
extern	struct	mapops	mmap_null;

struct	mapops	*vnode_mmap[] = {
	&mmap_null,		/* VNON */
	&mmap_reg,		/* VREG */
	&mmap_null,		/* VDIR */
	&mmap_null,		/* VBLK */
	&mmap_chr,		/* VCHR */
	&mmap_null,		/* VLNK */
	&mmap_null,		/* VSOCK */
	&mmap_null,		/* VFIFO */
	&mmap_null,		/* VBAD */
};

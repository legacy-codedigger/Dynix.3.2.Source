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

/* $Header: local.h 2.1 87/04/02 $ */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)local.h	5.1 (Berkeley) 6/6/85
 */

#ifdef V7
#include "v7.local.h"
#endif

#ifdef CORY
#include "c.local.h"
#endif

#ifdef INGRES
#include "ing.local.h"
#endif

#ifdef V6
#include "v6.local.h"
#endif

#ifdef CC
#include "cc.local.h"
#endif

#ifdef V40
#include "40.local.h"
#endif

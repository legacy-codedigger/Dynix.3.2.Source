/*
 * $Copyright:	$
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

	.file	"fabs.s"
	.text
	.asciz	"$Header: fabs.s 1.7 87/05/23 $"

/*
 * double fabs(x)
 *	double x;
 *
 * Double absolute value
 */

#include "DEFS.h"

ENTRY(fabs)
	ENTER

#ifdef	i387			/* Intel 80387 */
	fldl	8(%ebp)
	fabs
#endif

#ifdef	w1167			/* Weitek 1167 */
	wloadl	8(%ebp),%fp2
	wabsl	%fp2
#endif

#if !defined(i387) && !defined(w1167)
  @@@ COMPILE ERROR - missing code or defines
#endif

	EXIT
	RETURN

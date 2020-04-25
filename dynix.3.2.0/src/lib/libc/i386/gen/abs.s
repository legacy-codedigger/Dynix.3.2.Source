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

	.file	"abs.s"
	.text
	.asciz	"$Header: abs.s 1.4 86/06/19 $"

/*
 * abs(x)
 *
 * Return absolute value
 */

#include "DEFS.h"

ENTRY(abs)
	movl	SPARG0,%eax
	testl	%eax,%eax
	jge	abs_pos
	negl	%eax
abs_pos:
	RETURN

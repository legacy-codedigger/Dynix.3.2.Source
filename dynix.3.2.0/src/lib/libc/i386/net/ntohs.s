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

	.file	"ntohs.c"
	.text
	.asciz	"$Header: ntohs.s 1.3 86/05/11 $"

/*
 *
 * hostorder = ntohs(netorder)
 *
 * Rotates two lower bytes and clears upper two
 * NOTE: "xchg" is smaller than "rolw $8, %ax"
 */

#include "../DEFS.h"

ENTRY(ntohs)
	movzwl	SPARG0,%eax	# zero!zero!1!0 - 6 clocks
	xchgb	%ah,%al		# zero!zero!0!1 - 3 clocks
				# 	total =   9 clocks
	RETURN

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

	.file	"htonl.s"
	.text
	.asciz	"$Header: htonl.s 1.3 86/05/11 $"

/*
 *
 * netorder = htonl(hostorder)
 *
 * Rotates words and bytes in words
 * NOTE: "xchg" is smaller than "rolw $8, %ax"
 */

#include "../DEFS.h"

ENTRY(htonl)
	movl	SPARG0,%eax	# 3!2!1!0 - 4 clocks
	xchgb	%ah,%al		# 3!2!0!1 - 3 clocks
	roll	$16,%eax	# 0!1!3!2 - 3 clocks
	xchgb	%ah,%al		# 0!1!2!3 - 3 clocks
				# total =  13 clocks
	RETURN

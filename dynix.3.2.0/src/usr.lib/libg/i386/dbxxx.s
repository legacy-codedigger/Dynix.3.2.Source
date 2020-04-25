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

/*
 * dbxxx.s -- object support for symbolic debugging.
 */
	.file	"dbxxx.s"
	.globl	__dbsubc
	.globl	__dbsubn
	.globl	__dbdesc
	.comm	__dbargs,512
	.text
	.asciz 	"$Header: dbxxx.s 1.3 86/06/18 $"
__dbsubc:
	call	*__dbdesc
__dbsubn:
	int	3
__dbdesc:
	.long	0

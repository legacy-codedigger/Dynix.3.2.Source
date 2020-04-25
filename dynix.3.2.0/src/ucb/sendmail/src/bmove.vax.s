# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: bmove.vax.s 2.0 86/01/28 $

#
#  BMOVE.S -- optimized block move routine.
#
#	@(#)bmove.vax.s	4.1	7/25/83
#
.globl	_bmove
_bmove:
	.word	0x0030
	movc3	12(ap),*4(ap),*8(ap)
	ret

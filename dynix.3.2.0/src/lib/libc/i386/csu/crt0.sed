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

# $Header: crt0.sed 1.2 86/06/23 $

s/_mcount/mcount/g
s/_start/start/g
/^\//d


/^start/ {
	N
	N
	c\
start:\

}

/hlt/,/\.data/c\
\	hlt\

/^_rcsid/s//\	.text\
&/

/^	.globl	_environ/s//\	.data\
\	.align 2\
&/

/^	pushl	$42/s//.Lpc:	pushl	$.Lpc/

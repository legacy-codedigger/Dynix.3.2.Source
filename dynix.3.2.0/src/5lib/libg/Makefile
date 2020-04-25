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

# $Header: Makefile 1.1 87/01/15 $
#
# libg.a (sdb support library) Makefile
#
# even though libg.a is named like a library, it is really a plain object file.

DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AS	= $(ATT_AS)

all	:  libg.a
libg.a	:  $(MACHINE)/dbxxx.s
	   $(AS) -o libg.a $(MACHINE)/dbxxx.s
install	:; install -m 644 libg.a $(DESTDIR)/usr/lib/libg.a
clean	:; rm -f libg.a

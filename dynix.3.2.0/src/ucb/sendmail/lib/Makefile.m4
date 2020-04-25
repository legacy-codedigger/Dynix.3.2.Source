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

# $Header: Makefile.m4 2.1 87/04/28 $
#
#  Makefile for Sendmail library
#
include(../md/config.m4)dnl

ALL=	libsys.a
LIBOBJS=syslog.o

CCONFIG=m4CONFIG
CFLAGS=	-O -I../`include' ${CCONFIG} ${INCLUDE} # -DEBUG
ASMSED=	../`include'/asm.sed

all: $(ALL)

libsys.a: $(LIBOBJS)
	${AR} ruv libsys.a $?
	${RANLIB} libsys.a

# Only used if not using 4.2 BSD
ndir:	FRC
	cd libndir; make ${MFLAGS} ${MRULES}
	${AR} ruv libsys.a libndir/*.o
	${RANLIB} libsys.a
	rm -f ../`include'/dir.h
	cp libndir/dir.h ../`include'

FRC:

clean:
	rm -f libsys.a core a.out Makefile # m4!
	rm -f *.o libndir/*.o

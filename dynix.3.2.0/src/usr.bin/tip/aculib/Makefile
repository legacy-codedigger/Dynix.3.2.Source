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

# $Header: Makefile 2.2 89/10/27 $
#
# make file for tip device drivers
#
# Current drivers:
#	BIZCOMP
#	DEC DF02-AC, DF03-AC
#	DEC DN-11/Able Quadracall
#	HAYES SMARTMODEM 1200
#       TELEBIT TRAILBLAZER (TB+/T1000)
#	VENTEL 212+ (w/o echo)
#	VADIC 831 RS232 adaptor
#	VADIC 3451
#
CFLAGS=	-O -I../ ${INCLUDE}
#

OBJS=	biz22.o biz31.o df.o dn11.o hayes.o telebit.o v3451.o v831.o ventel.o

aculib.a:& ${OBJS}
	${AR} cru aculib.a ${OBJS}
	${RANLIB} aculib.a

${OBJS}: ../tip.h

clean:
	rm -f aculib.a *.o core errs a.out

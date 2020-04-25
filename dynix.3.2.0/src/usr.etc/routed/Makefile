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

# $Header: Makefile 2.5 87/04/20 $
#
CFLAGS=	-O ${INCLUDE}
#
OBJS=	af.o if.o input.o main.o output.o startup.o tables.o timer.o \
	trace.o inet.o

all:	routed

routed:& ${OBJS}
	${CC} -o routed ${OBJS}

install:
	install -s routed ${DESTDIR}/usr/etc

clean:
	rm -f routed *.o core a.out t.c linterrs typescript errs

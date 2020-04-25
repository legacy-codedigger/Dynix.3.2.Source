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

# $Header: Makefile 2.0 86/01/28 $
#
CFLAGS=	-O ${INCLUDE}
#

OBJS=	regexp.o vfontedpr.o vgrindefs.o

all:	vfontedpr retest

vfontedpr: ${OBJS}
	${CC} -o vfontedpr ${OBJS}

retest: retest.o regexp.o
	${CC} -o retest retest.o regexp.o

install:
	install -s vfontedpr ${DESTDIR}/usr/lib/vfontedpr
	install -c vgrind.sh ${DESTDIR}/usr/ucb/vgrind
	install -c vgrindefs.src ${DESTDIR}/usr/lib/vgrindefs
	install -c -m 444 tmac.vgrind ${DESTDIR}/usr/lib/tmac/tmac.vgrind

clean:
	rm -f vfontedpr retest ${OBJS} errs core a.out *.o

vprintall:
	csh /usr/ucb/vprint ${SOURCES}

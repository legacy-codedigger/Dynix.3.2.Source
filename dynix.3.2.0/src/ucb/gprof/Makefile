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
# add -DCFLAG to CFLAGS and compile calls.c to enable -c option

DFLAGS = 
CFLAGS = -O ${DFLAGS} ${INCLUDE}
LINTFLAGS = -x -a -n ${DFLAGS}

RM=	/bin/rm -f
PR=pr

GPROFHDRS =	gprof.h ns32000.h
GPROFSRCS =	gprof.c arcs.c dfn.c lookup.c hertz.c \
		printgprof.c printlist.c
#		calls.c 
GPROFOBJS =	gprof.o arcs.o dfn.o lookup.o hertz.o \
		printgprof.o printlist.o
#		calls.o 

all:	gprof gprof.flat.blurb gprof.callg.blurb

gprof:& ${GPROFOBJS}
	${CC} -o gprof ${GPROFOBJS}

install:
	install -o root -g daemon -m 2755 -s gprof ${DESTDIR}/usr/ucb/gprof
	cp gprof.flat.blurb ${DESTDIR}/usr/lib
	cp gprof.callg.blurb ${DESTDIR}/usr/lib

clean:
	${RM} ${GPROFOBJS} errs gprof

#gcrt0.h:
#	cp /usr/src/libc/csu/gcrt0.h gcrt0.h

print:
	@ ls -l | ${PR}
	@ ${PR} makefile
	@ ${PR} gcrt0.h
	@ ${PR} ${GPROFHDRS} ${GPROFSRCS}
	@ ${PR} gprof.flat.blurb gprof.callg.blurb

lint:
	lint ${LINTFLAGS} ${DFLAGS} ${GPROFSRCS}

gprof.o: gprof.c gprof.h vax.h gcrt0.h
arcs.o: arcs.c gprof.h vax.h gcrt0.h
lookup.o: lookup.c gprof.h vax.h gcrt0.h
dfn.o: dfn.c gprof.h vax.h gcrt0.h
#calls.o: calls.c gprof.h vax.h gcrt0.h
hertz.o: gprof.h vax.h hertz.c
printgprof.o: printgprof.c gprof.h vax.h gcrt0.h
printlist.o: printlist.c gprof.h vax.h gcrt0.h

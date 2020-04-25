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
# make file for cribbage
#
CFLAGS=	-O ${INCLUDE}
#
HEADRS=	cribbage.h deck.h cribcur.h
OBJS=	extern.o crib.o support.o cards.o score.o io.o
CFILES=	extern.c crib.c support.c cards.c score.c io.c
TOBJS=	test.o cards.o score.o io.o extern.o
#

all:	cribbage crib.instr

cribbage: ${OBJS}
	${CC} ${CFLAGS} -o cribbage ${OBJS} -lcurses -ltermlib

crib.o io.o support.o: cribcur.h

test:	${TOBJS}
	${CC} ${CFLAGS} -o test ${TOBJS} -lcurses -ltermlib

crib.instr: cribbage.n macro
	nroff -Tcrt cribbage.n > crib.instr

install:
	install -s cribbage   ${DESTDIR}/usr/games/cribbage
	install -c crib.instr ${DESTDIR}/usr/games/lib/crib.instr

tags: ${HEADRS} ${CFILES}
	ctags -u $?
	ed - tags < :ctfix
	sort tags -o tags

clean:
	rm -f ${OBJS} ? a.out core crib.instr cribbage

crib.o:		deck.h	cribbage.h	cribcur.h
support.o:	deck.h	cribbage.h	cribcur.h
test.o:		deck.h
cards.o:	deck.h
score.o:	deck.h
io.o:		deck.h			cribcur.h

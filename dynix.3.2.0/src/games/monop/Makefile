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
# Nifty foo monopoly maker
#	Program:  Ken Arnold
#	Makefile:  K.S.
#
CFLAGS=	-O ${INCLUDE}
#
DATFILES=brd.dat mon.dat prop.dat
CFILES=	monop.c cards.c execute.c getinp.c houses.c jail.c misc.c morg.c \
	print.c prop.c rent.c roll.c spec.c strcmp.c trade.c
OBJS=	monop.o cards.o execute.o getinp.o houses.o jail.o misc.o morg.o \
	print.o prop.o rent.o roll.o spec.o strcmp.o trade.o strings.o
HEADERS=monop.h deck.h
BINDIR=	${DESTDIR}/usr/games
LIBDIR=	${DESTDIR}/usr/games/lib
LIB=
DAT=	brd.dat monop.dat prop.dat
XSTR=	xstr
ED=	ed
RM=	rm -f
CTAGS=	ctags
#

#
# Be cool about compiling strings.
#
.c.o:
	${CC} -E ${CFLAGS} $*.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} x.c
	mv x.o $*.o

all:	strings monop cards.pck

strings:
	touch strings
	chmod +w strings

monop:	${OBJS}
	${CC} -o monop ${OBJS} ${LIB}

install:
	${RM} -f ${BINDIR}/monop ${LIBDIR}/cards.pck
	install -s monop ${BINDIR}
	install cards.pck ${LIBDIR}

strings.o: strings
	${XSTR}
	${CC} -S xs.c
	${ED} - < :rofix xs.s
	${AS} -o strings.o xs.s
	${RM} xs.s xs.c

monop.o: ${DATFILES} monop.def
	${CC} -E ${CFLAGS} monop.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} x.c
	mv x.o monop.o

cards.o: deck.h
	${CC} -E ${CFLAGS} cards.c | ${XSTR} -c -
	${CC} -c ${CFLAGS} x.c
	mv x.o cards.o

newone:
	${CC} -o monop ${CFLAGS} ${CFILES} ${LIB}

ctags:
	${CTAGS} ${CFILES} monop.h deck.h monop.def initdeck.c

cards.pck: initdeck cards.inp
	./initdeck

initdeck: initdeck.c deck.h
	cc -o initdeck ${CFLAGS} initdeck.c ${LIB}

monop.tar:
	tar crvf monop.tar ${CFILES} ${DATFILES} ${HEADERS} :rofix Makefile cards.inp initdeck.c monop.def monop.ext

clean:
	${RM} ${OBJS} monop core tags x.c x.o monop.tar strings initdeck

printall:
	-/usr/bin/eecs/pr monop.h deck.h monop.def monop.ext ${CFILES} ${DATFILES} | /usr/bin/lpr
	-/usr/bin/lpq

lint:
	lint ${CFILES} > lint.out ;

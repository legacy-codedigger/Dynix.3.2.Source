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
# mille bourne game makefile
#
HEADERS=mille.h
CFILES=	comp.c end.c extern.c init.c mille.c misc.c move.c print.c \
	roll.c save.c types.c varpush.c
OBJS=	comp.o end.o extern.o init.o mille.o misc.o move.o print.o \
	roll.o save.o types.o varpush.o
POBJS=	comp.po end.po extern.po init.po mille.po misc.po move.po \
	roll.po print.po save.po types.po varpush.po
#
CRL=	
L=	-lcurses -ltermlib
MLIBS=	${CRL} ${L}
DEFS=	
CFLAGS=	-O ${DEFS} ${INCLUDE}
BINDIR=	${DESTDIR}/usr/games
.SUFFIXES: .po .i

.c.po:
	rm -f x.c ; ln $*.c x.c
	${CC} ${CFLAGS} -p -c x.c
	mv x.o $*.po

.c.i:
	${CC} ${CFLAGS} -P $*.c

all:	mille

mille:	${OBJS} ${CRL}
	${CC} ${CFLAGS} -o mille ${OBJS} ${MLIBS}

install:
	install -s mille ${BINDIR}/mille

pmb: ${POBJS} ../pcrlib
	${CC} ${CFLAGS} -p -o pmb ${POBJS} ../pcrlib -ltermlib

mille.po: mille.c
	rm -f x.c ; ln mille.c x.c
	${CC} ${CFLAGS} -DPROF -p -c x.c
	mv x.o mille.po

table: table.o extern.o
	${CC} ${CFLAGS} -i -o table table.o extern.o

readdump: readdump.o extern.o varpush.o
	${CC} ${CFLAGS} -i -o readdump readdump.o extern.o varpush.o

ctags:
	ctags ${HEADERS} ${CFILES}
	ed - tags < :ctfix
	sort tags -o tags

lint:
	lint -hxb ${DEFS} ${CFILES} ${L} > lint.out

mille.ar:
	${AR} ruv mille.ar Makefile tags ${HEADERS} ${CFILES}

tar:
	tar rvf /dev/rmt0 Makefile tags :ctfix ${HEADERS} ${CFILES}

lpr:
	pr Makefile ${HEADERS} ${CFILES} tags | lpr ; lpq

clean:
	rm -f ${OBJS} ${POBJS} core ? a.out mille

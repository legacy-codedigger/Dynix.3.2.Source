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

CFLAGS= -O -DV7 ${INCLUDE}
#
OBJS=	allow.o board.o check.o extra.o fancy.o init.o main.o move.o\
	odds.o one.o save.o subs.o table.o text.o message.o
TOBJS=	allow.o board.o check.o data.o fancy.o init.o odds.o one.o\
	save.o subs.o table.o teach.o ttext1.o ttext2.o tutor.o

all:	Message backgammon teachgammon backgammon.doc

#	Backgammon program
backgammon: $(OBJS)
	rm -f backgammon
	${CC} -o backgammon $(OBJS) -ltermlib

#	Backgammon rules and tutorial
teachgammon: ${TOBJS}
	rm -f teachgammon
	${CC} -o teachgammon $(TOBJS) -ltermlib

#	Header files back.h and tutor.h
allow.o board.o check.o extra.o fancy.o main.o move.o odds.o one.o save.o\
	subs.o table.o teach.o text.o ttext1.o ttext2.o: back.h
data.o tutor.o: back.h tutor.h

#	Update message.
Message: FRC
#	@chmod +w message.c
#	ex - message.c < Mesgfix
	touch Message
#	@chmod -w message.c

FRC:

#	Documentation
backgammon.doc: backgammon.src
	rm -f backgammon.doc
	nroff -man -Tcrt backgammon.src > backgammon.doc

#	Installation
install:
	install -s backgammon  ${DESTDIR}/usr/games/backgammon
	install -s teachgammon ${DESTDIR}/usr/games/teachgammon

clean:
	rm -f ${OBJS} ${TOBJS} Message backgammon teachgammon backgammon.doc

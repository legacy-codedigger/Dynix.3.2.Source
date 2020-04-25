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
YFLAGS=	-d
#

OBJS=	e.o diacrit.o eqnbox.o font.o fromto.o funny.o glob.o\
	integral.o io.o lex.o lookup.o mark.o matrix.o move.o\
	over.o paren.o pile.o shift.o size.o sqrt.o text.o

.c.o:
	${CC} ${CFLAGS} -DNEQN -c $*.c
	mv $*.o n/$*.o
	${CC} ${CFLAGS} -UNEQN -c $*.c

all:	n eqn neqn

n:	
	mkdir n

eqn:	${OBJS}
	${CC} -o eqn ${OBJS}

neqn:	${OBJS}
	cd n; ${CC} -o ../neqn ${OBJS}

e.c:	e.def

e.def:	e.y
	${YACC} -d e.y
	mv y.tab.c e.c
	mv y.tab.h e.def

${OBJS}: e.h e.def

install:
	install -s eqn  ${DESTDIR}/usr/bin
	install -s neqn ${DESTDIR}/usr/bin

clean:
	rm -f *.o e.c e.def eqn
	rm -rf n

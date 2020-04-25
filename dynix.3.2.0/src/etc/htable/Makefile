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

# $Header: Makefile 2.1 90/07/20 $
# @(#)Makefile	4.6 11/3/83
CFLAGS=	-O ${INCLUDE}
ALL=	htable

all:	${ALL}

htable:&	htable.o parse.o scan.o
	${CC} htable.o parse.o scan.o -o htable

htable.o: htable.c htable.h
	${CC} ${CFLAGS} -c htable.c

scan.o:	scan.c htable.h y.tab.h

parse.o: parse.c htable.h

scan.c:	scan.l

parse.c: parse.y

y.tab.h:
	${YACC} -d parse.y
	rm y.tab.c

clean:
	rm -f ${ALL} scan.c parse.c *.o *.s y.tab.h errs

install:
	install -s htable ${DESTDIR}/etc/htable

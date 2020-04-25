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

OBJS=	lmain.o y.tab.o sub1.o sub2.o header.o

all:	lex

lex:	${OBJS}
	${CC} -o lex ${OBJS}

smallex:
	${CC} ${CFLAGS} -DSMALL \
		lmain.c y.tab.c sub1.c sub2.c header.c -o smallex

y.tab.c:	parser.y; ${YACC} parser.y
lmain.o:	lmain.c ldefs.c once.c
sub1.o:		sub1.c ldefs.c
sub2.o:		sub2.c ldefs.c
header.o:	header.c ldefs.c

install:
	install -s lex ${DESTDIR}/usr/bin
	install -c ncform $(DESTDIR)/usr/lib/lex
	install -c nrform $(DESTDIR)/usr/lib/lex

clean:
	rm -f *.o lex y.tab.c

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

all: 	compact uncompact

compact: compact.o tree.o
	${CC} -o compact compact.o tree.o

uncompact: uncompact.o tree.o
	${CC} -o uncompact uncompact.o tree.o

compact.o uncompact.o tree.o: compact.h

install:
	install -s compact   ${DESTDIR}/usr/ucb/compact
	install -s uncompact ${DESTDIR}/usr/ucb/uncompact
	install -c -m 755 ccat.sh ${DESTDIR}/usr/ucb/ccat

clean:
	rm -f compact.o uncompact.o tree.o

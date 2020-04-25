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

# $Header: Makefile 2.3 90/05/08 $
#
CFLAGS=	-O ${INCLUDE}
#
FORTUNES=	scene obscene
LIBDIR	=	/usr/games/lib
BINDIR	=	/usr/games
OWN	=	daemon
#

all:	fortune strfile unstr fortunes.dat

fortune: strfile.h fortune.c
	${CC} ${CFLAGS} -DFORTFILE='"${LIBDIR}/fortunes.dat"' -o fortune fortune.c

strfile: strfile.h strfile.c
	${CC} ${CFLAGS} -o strfile strfile.c
	cc ${CFLAGS} -o strfile.loc strfile.c

unstr: strfile.h unstr.c
	${CC} ${CFLAGS} -o unstr unstr.c

fortunes.dat: fortunes strfile
	./strfile.loc fortunes

fortunes: $(FORTUNES)
	cat scene > fortunes
	echo "%-" >> fortunes
	cat obscene >> fortunes
	echo "%%" >> fortunes

install:
	install -m 0600 -o ${OWN} fortunes.dat ${DESTDIR}/${LIBDIR}
	install -s -m 0600 -o ${OWN} strfile ${DESTDIR}/${LIBDIR}
	install -s -m 4711 -o ${OWN} fortune ${DESTDIR}/${BINDIR}

clean:
	rm -f fortune fortunes fortunes.dat fortunes.tar strfile.loc strfile unstr

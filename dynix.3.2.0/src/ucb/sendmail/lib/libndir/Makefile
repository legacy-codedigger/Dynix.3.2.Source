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

CFLAGS=	-O -I. ${INCLUDE}

OBJS=	closedir.o opendir.o readdir.o seekdir.o telldir.o

.c.o:
	${CC} ${CFLAGS} -c $*.c
	-ld -x -r $*.o
	mv a.out $*.o

all:	libndir

libndir: ${OBJS}
	ar cru libndir ${OBJS}
	ranlib libndir

install:
	cp dir.h ${DESTDIR}/usr/include/dir.h
	cp libndir ${DESTDIR}/usr/lib/libndir.a
	ranlib ${DESTDIR}/usr/lib/libndir.a
	cp directory.3s /usr/man/man3/directory.3s

clean:
	rm -f libndir ${OBJS}

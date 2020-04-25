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

# $Header: Makefile 1.3 90/02/23 $
#
CFLAGS=	-O ${INCLUDE}

OBJS=	newfs.o mkfs.o
LIBS=	-lseq

all:	newfs

newfs:&	${OBJS}
	${CC} ${CFLAGS} -o newfs ${OBJS} ${LIBS}

install:
	install -s newfs ${DESTDIR}/etc
	ln ${DESTDIR}/etc/newfs ${DESTDIR}/etc/mkfs

clean:
	rm -f newfs ${OBJS} errs a.out core

lint:
	lint ${INCLUDE} newfs.c mkfs.c

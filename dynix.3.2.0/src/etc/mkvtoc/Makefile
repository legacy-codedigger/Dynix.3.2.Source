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

# $Header: Makefile 1.4 90/08/06 $
#
CFLAGS=	-O ${INCLUDE}
#
OBJS=	mkvtoc.o
LIBS= -lseq

all:	mkvtoc

mkvtoc:&	${OBJS}
	${CC} ${CFLAGS} -o mkvtoc ${OBJS} ${LIBS}

install: mkvtoc
	install -s mkvtoc ${DESTDIR}/etc
	-rm -f ${DESTDIR}/etc/vtoc/*
	cd vtoc; install -c -m 644 * ${DESTDIR}/etc/vtoc

clean:
	rm -f mkvtoc ${OBJS} errs a.out core

lint:
	lint mkvtoc.c

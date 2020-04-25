# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.3 1991/04/12 22:43:04 $

CFLAGS=-O  ${INCLUDE}
#CFLAGS=-O
LIBC=	/lib/libc.a
LIBSEQ= -lseq
SRCS=	savecore.c pcopy.c compress.c
OBJS=	savecore.o pcopy.o compress.o

all: savecore

savecore:&	${OBJS} ${LIBC}
	${CC} -o $@ ${CFLAGS} ${OBJS} $(LIBSEQ)

clean: FRC
	rm -f ${OBJS} core savecore

depend: FRC
	mkdep ${CFLAGS} ${SRCS}

install: FRC
	install -s -m 2755 -g daemon savecore ${DESTDIR}/usr/etc/savecore
lint: FRC
	lint ${CFLAGS} ${SRCS}

tags: FRC
	ctags ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

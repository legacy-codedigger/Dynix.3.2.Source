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

CFILES=	done.c init.c io.c main.c save.c subr.c vocab.c wizard.c
OBJS=	done.o init.o io.o main.o save.o subr.o vocab.o wizard.o
HFILES = hdr.h
CFLAGS=	-O ${INCLUDE}
LDFLAGS=	# should be -i on small machines, -n on a vax

all:	advent

advent:	a.out
	rm -f advent
	echo y | ./a.out

a.out: ${OBJS}
	${CC} ${LDFLAGS} ${CFLAGS} ${OBJS}

save.o:	save.c	# vax C optimizer bug - infinite loop
	${CC} -c save.c

install:
	install -s advent ${DESTDIR}/usr/games/adventure

clean:
	rm -f ${OBJS} a.out advent core errs

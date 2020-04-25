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

# $Header: Makefile 1.1 90/02/22 $
#
CFLAGS=	-O ${INCLUDE}
#
OBJS=	rmvtoc.o
LIBS=

all:	rmvtoc

rmvtoc:	${OBJS}
	${CC} ${CFLAGS} -o rmvtoc ${OBJS} ${LIBS}

install: rmvtoc
	install -s rmvtoc ${DESTDIR}/usr/etc

clean:
	rm -f rmvtoc ${OBJS} errs a.out core

lint:
	lint ${INCLUDE} rmvtoc.c

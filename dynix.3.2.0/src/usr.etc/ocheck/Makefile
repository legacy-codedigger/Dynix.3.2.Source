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

# $Header: Makefile 1.1 90/02/12 $
#
CFLAGS=	-O ${INCLUDE}
#
LIBS=

all:	icheck dcheck ncheck

ncheck: ncheck.o alloc.o
	${CC} ${CFLAGS} -o ncheck ncheck.o alloc.o ${LIBS}

dcheck: dcheck.o alloc.o
	${CC} ${CFLAGS} -o dcheck dcheck.o alloc.o ${LIBS}

icheck: icheck.o alloc.o
	${CC} ${CFLAGS} -o icheck icheck.o alloc.o ${LIBS}

install: all
	install -s ncheck icheck dcheck ${DESTDIR}/usr/etc

clean:
	rm -f icheck dcheck ncheck *.o errs a.out core

lint:
	lint icheck.c alloc.c
	lint dcheck.c alloc.c
	lint ncheck.c alloc.c

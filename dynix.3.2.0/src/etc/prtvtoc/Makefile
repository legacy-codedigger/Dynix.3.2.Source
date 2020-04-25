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

# $Header: Makefile 1.1 90/01/23 $
#
CFLAGS=	-O ${INCLUDE}
#
OBJS=	prtvtoc.o
LIBS= -lseq

all:	prtvtoc

prtvtoc:&	${OBJS}
	${CC} ${CFLAGS} -o prtvtoc ${OBJS} ${LIBS}

install:
	install -s prtvtoc ${DESTDIR}/etc

clean:
	rm -f prtvtoc ${OBJS} errs a.out core

lint:
	lint prtvtoc.c

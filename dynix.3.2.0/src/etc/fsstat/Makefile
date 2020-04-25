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
OBJS=	fsstat.o
LIBS= -lseq

all:	fsstat

fsstat:&	${OBJS}
	${CC} ${CFLAGS} -o fsstat ${OBJS} ${LIBS}

install:
	install -s fsstat ${DESTDIR}/etc

clean:
	rm -f fsstat ${OBJS} errs a.out core

lint:
	lint fsstat.c

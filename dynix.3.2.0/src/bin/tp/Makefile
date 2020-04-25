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
CFLAGS=	-O ${INCLUDE}
#

OBJS=	tp0.o tp1.o tp2.o tp3.o

all:	tp

tp:&	${OBJS}
	${CC} -o tp ${OBJS}

${OBJS}: tp.h tp_defs.h

install:
	install -s -m 755 tp ${DESTDIR}/bin

clean: 
	rm -f ${OBJS} core a.out tp

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

# $Header: Makefile 1.2 89/10/09 $
#

#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation, advertising
# materials, and other materials related to such redistribution and
# use acknowledge that the software was developed by the University
# of California, Berkeley.  The name of the University may not be
# used to endorse or promote products derived from this software
# without specific prior written permission.  THIS SOFTWARE IS PROVIDED
# ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
# WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE.
#
# @(#)Makefile	5.2 (Berkeley) 5/9/89
#

CFLAGS=	-O $(INCLUDE)
LIBC=	/lib/libc.a
SRCS=	rshd.c
OBJS= 	rshd.o
MAN=	rshd.0

all: rshd

rshd: ${LIBC} 
	${CC} -o $@ ${CFLAGS} $@.c -lseq

clean:
	rm -f ${OBJS} core rshd

cleandir: clean
	rm -f tags .depend

depend: ${SRCS}
	mkdep -p ${CFLAGS} ${SRCS}

install:
	install -s -m 755 rshd ${DESTDIR}/usr/etc

lint: ${SRCS}
	lint ${CFLAGS} ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

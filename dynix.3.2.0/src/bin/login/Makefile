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
# @(#)Makefile	5.3 (Berkeley) 5/9/89
#

CFLAGS= -O -w ${INCLUDE}
SRCS=	login.c sysv.c
OBJS=	login.o sysv.o
MAN=

all: login

login: & $(OBJS) pathnames.h
	$(CC) -o $@ $(CFLAGS) $(OBJS) -lutil -lseq

clean:
	rm -f ${OBJS} core login

cleandir: clean
	rm -f ${MAN} tags .depend

install:
	install -s -o root -g daemon -m 4755 login ${DESTDIR}/bin/login

lint: ${SRCS}
	lint ${INCLUDE} ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

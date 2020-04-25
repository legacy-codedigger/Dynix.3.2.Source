#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile	5.7 (Berkeley) 6/18/88
#
VPATH=	../../ucb/tftp
CFLAGS=	-O ${INCLUDE}
LIBC=	/lib/libc.a
SRCS=	tftpd.c tftpsubs.c
OBJS=	tftpd.o tftpsubs.o

all: tftpd

tftpd:&	${OBJS} ${LIBC}
	${CC} -o $@ ${CFLAGS} ${OBJS} -lseq

clean: FRC
	rm -f ${OBJS} core tftpd

depend: FRC
	mkdep ${CFLAGS} ${SRCS}

install: FRC
	install -s tftpd ${DESTDIR}/usr/etc/tftpd

lint: FRC
	lint ${CFLAGS} ${SRCS}

tags: FRC
	ctags ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

#  $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.
#

#ifndef	lint
static char rcsid[] = "$Header: Makefile 1.2 90/07/08 $";
#endif

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
CFLAGS=	-O ${INCLUDE}
LIBC=	/lib/libc.a
SRCS=	talkd.c announce.c process.c table.c print.c
OBJS=	talkd.o announce.o process.o table.o print.o

all: talkd

talkd:&	${OBJS} ${LIBC}
	${CC} -o $@ ${CFLAGS} ${OBJS}

clean: FRC
	rm -f ${OBJS} core talkd

depend: FRC
	mkdep ${CFLAGS} ${SRCS}

install: FRC
	install -s talkd ${DESTDIR}/usr/etc/talkd

lint: FRC
	lint ${CFLAGS} ${SRCS}

tags: FRC
	ctags ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

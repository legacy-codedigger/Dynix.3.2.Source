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

#ident	"$Header: Makefile 1.3 90/06/08 $"

#  $Log:	Makefile,v $

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
#	@(#)Makefile	5.19 (Berkeley) 10/10/88
#
# The file nslookup.help is now installed by proto.sh
# changes should go there.

CFLAGS= -O $(INCLUDE)
RES=
LIBC=	/lib/libc.a
LIBS=	-ll ${RES}
CSRCS=	main.c getinfo.c debug.c send.c skip.c list.c subr.c
SRCS=	${CSRCS} commands.c
OBJS=	main.o getinfo.o debug.o send.o skip.o list.o subr.o commands.o

nslookup:& ${OBJS} ${LIBC}
	${CC} -o $@ ${OBJS} ${LIBS}

clean: FRC
	rm -f ${OBJS} core nslookup commands.c lex.yy.c lex.yy.o

depend: ${SRCS} FRC
	mkdep ${CFLAGS} ${SRCS}

install: FRC
	install -s -m 755 nslookup ${DESTDIR}/usr/etc/nslookup

lint: ${SRCS} FRC
	lint ${CFLAGS} ${SRCS}

tags: ${CSRCS} FRC
	ctags ${CSRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

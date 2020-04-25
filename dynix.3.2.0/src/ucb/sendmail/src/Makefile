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

# $Header: Makefile 1.5 90/09/03 $
#
# Copyright (c) 1988 Regents of the University of California.
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
#	@(#)Makefile	5.11 (Berkeley) 1/3/89
#
SRCS=	alias.c arpadate.c clock.c collect.c conf.c convtime.c daemon.c \
	deliver.c domain.c envelope.c err.c headers.c macro.c main.c \
	parseaddr.c queue.c readcf.c recipient.c savemail.c srvrsmtp.c \
	stab.c stats.c sysexits.c trace.c usersmtp.c util.c Version.c
OBJS=	alias.o arpadate.o clock.o collect.o conf.o convtime.o daemon.o \
	deliver.o domain.o envelope.o err.o headers.o macro.o main.o \
	parseaddr.o queue.o readcf.o recipient.o savemail.o srvrsmtp.o \
	stab.o stats.o sysexits.o trace.o usersmtp.o util.o

CFLAGS=	-O -I. -I../include ${INCLUDE} -DVMUNIX

GET=	sccs get
DELTA=	sccs delta
WHAT=	what
PRT=	sccs prt

sendmail:& ${OBJS} Version.o
	${CC} ${COPTS} -o $@ Version.o ${OBJS} -ldbm
	xsize sendmail; ls -l sendmail; ${WHAT} < Version.o

dsendmail: ${OBJS} Version.o
	${CC} ${COPTS} -o sendmail Version.o ${OBJS} -ldbm -lresolv
	xsize sendmail; ls -l sendmail; ${WHAT} < Version.o

install: 
	install -s -m 4755 sendmail ${DESTDIR}/usr/lib
#	install -c -o bin -g bin -m 644 /dev/null ${DESTDIR}/usr/lib/sendmail.fc
#	install -c -o bin -g bin -m 666 /dev/null ${DESTDIR}/usr/lib/sendmail.st
#	install -c -o bin -g bin -m 666 /dev/null ${DESTDIR}/usr/lib/sendmail.st
#	install -c -o bin -g bin -m 444 sendmail.hf /usr/lib/sendmail.hf
#	install -c -o bin -g bin -m 444 sendmail.0 ${DESTDIR}/usr/man/cat8
#	install -c -o bin -g bin -m 444 aliases.0 ${DESTDIR}/usr/man/cat5
#	install -c -o bin -g bin -m 444 newaliases.0 ${DESTDIR}/usr/man/cat1
#	-mkdir ${DESTDIR}/usr/spool/mqueue
#	chown root.wheel ${DESTDIR}/usr/spool/mqueue
#	chmod 775 ${DESTDIR}/usr/spool/mqueue
#	rm -f ${DESTDIR}/usr/ucb/newaliases
#	ln -s ${DESTDIR}/usr/lib/sendmail ${DESTDIR}/usr/ucb/newaliases
#	rm -f ${DESTDIR}/usr/ucb/mailq
#	ln -s ${DESTDIR}/usr/lib/sendmail ${DESTDIR}/usr/ucb/mailq

version: newversion ${OBJS} Version.c

newversion:
	@rm -f SCCS/p.version.c version.c
	@${GET} -e SCCS/s.version.c
	@${DELTA} -s SCCS/s.version.c
	@${GET} -t -s SCCS/s.version.c

clean: ${SRCS}
	rm -f ${OBJS} core sendmail a.out Version.o

cleandir: clean
	rm -f tags .depend

depend: ${SRCS}
	mkdep ${CFLAGS} ${SRCS}

tags: ${SRCS}
	ctags ${SRCS}

lint: ${SRCS}
	lint -chapbx ${CCONFIG} ${SRCS}

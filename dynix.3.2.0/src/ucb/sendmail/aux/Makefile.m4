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

# $Header: Makefile.m4 2.1 87/04/28 $
#
#  Makefile for assorted programs related {perhaps distantly} to Sendmail.
#
#  Note: syslog.conf is now installed by proto.sh
include(../md/config.m4)dnl

ALL=	syslog
NOTYET=	praliases vacation logger mconnect

SLIBS=	../lib/libsys.a m4LIBS
DBMLIB=	-ldbm
CHMOD=	chmod
O=	-O
COPTS=
CCONFIG=-I../`include' -DDBM -DLOG m4CONFIG # -DDEBUG
CFLAGS=	$O ${COPTS} ${CCONFIG} ${INCLUDE}
ASMSED=	../`include'/asm.sed
ARFLAGS=rvu
LINT=	lint
XREF=	ctags -x
CP=	cp
MV=	mv
M4=	m4
TOUCH=	touch
ABORT=	false

OBJMODE=755

all: ${ALL}

logger: logger.o
	${CC} ${COPTS} -o $@ $*.o ${SLIBS}

mconnect: mconnect.o
	${CC} ${COPTS} -o $@ $*.o

praliases: praliases.o
	${CC} ${COPTS} -o $@ $*.o

syslog: syslog.o
	${CC} ${COPTS} -o $@ $*.o

vacation: vacation.o ../src/convtime.o
	${CC} ${COPTS} -o $@ $@.o ../src/convtime.o ${DBMLIB}

../src/convtime.o:
	cd ../src; \
		m4 Makefile.m4 > Makefile; \
		make ${MFLAGS} ${MRULES} convtime.o

install: all
	install -s -o daemon syslog ${DESTDIR}/etc/syslog
	install -c -o daemon -m 644 /dev/null ${DESTDIR}/etc/syslog.pid
#	install -s -o daemon logger ${DESTDIR}/etc/logger
#	install vacation ${DESTDIR}/usr/ucb/vacation
#	install -o daemon praliases ${DESTDIR}/etc/praliases
	
clean:
	rm -f ${ALL} core a.out make.out lint.out Makefile
	rm -f *.o

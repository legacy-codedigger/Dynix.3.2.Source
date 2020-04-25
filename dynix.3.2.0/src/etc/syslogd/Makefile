# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.2 91/01/23 $
#
CFLAGS=	-O ${INCLUDE} 
#

all:	syslogd

syslogd: syslogd.c pathnames.h
	${CC} ${CFLAGS} -o syslogd syslogd.c

install:
	install -s syslogd ${DESTDIR}/etc

clean clobber:
	rm -f syslogd 

lint:
	lint ${CFLAGS} syslogd.c

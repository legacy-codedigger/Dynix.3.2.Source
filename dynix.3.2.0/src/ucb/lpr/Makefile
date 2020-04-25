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

# $Header: Makefile 2.1 87/01/06 $
#
# makefile for line printer spooling system
#
# Ownerships {see note at the top of lpd.c}
# ROOT		a user that can access any and all files
# DAEMON	someone special
# SPGRP		the group id of the spooling programs
#
CFLAGS=-O ${INCLUDE}
LIBDIR=/usr/lib
BINDIR=/usr/ucb
SPOOLDIR=/usr/spool/lpd
CXREF=/usr/ucb/ctags -x
DAEMON=daemon
SPGRP=daemon
#

ALL=	lpd lpr lpq lprm pac lpc

all:&	${ALL} FILTERS

lpd:	lpd.o printjob.o recvjob.o displayq.o rmjob.o
lpd:	startdaemon.o lpdchar.o common.o printcap.o
	${CC} -o lpd lpd.o printjob.o recvjob.o displayq.o rmjob.o \
		startdaemon.o lpdchar.o common.o printcap.o

lpr:	lpr.o startdaemon.o printcap.o
	${CC} -o lpr lpr.o startdaemon.o printcap.o

lpq:	lpq.o displayq.o common.o printcap.o
	${CC} -o lpq lpq.o displayq.o common.o printcap.o -ltermcap

lprm:	lprm.o rmjob.o startdaemon.o common.o printcap.o
	${CC} -o lprm lprm.o rmjob.o startdaemon.o common.o printcap.o

lpc:	lpc.o cmds.o cmdtab.o startdaemon.o common.o printcap.o
	${CC} -o lpc lpc.o cmds.o cmdtab.o startdaemon.o common.o printcap.o

pac:	pac.o printcap.o
	${CC} -o pac pac.o printcap.o

lpd.o lpr.o lpq.o lprm.o pac.o: lp.h lp.local.h
recvjob.o printjob.o displayq.o rmjob.o common.o: lp.h lp.local.h
startdaemon.o: lp.local.h
lpc.o cmdtab.o: lpc.h
cmds.o: lp.h lp.local.h

FILTERS:
	cd filters; make ${MFLAGS} ${MRULES}

install:
	install -s -g ${SPGRP} -m 6711 lpd ${DESTDIR}/${LIBDIR}/lpd
	install -s -g ${SPGRP} -m 6711 lpr ${DESTDIR}/${BINDIR}/lpr
	install -s -g ${SPGRP} -m 6711 lpq ${DESTDIR}/${BINDIR}/lpq
	install -s -g ${SPGRP} -m 6711 lprm ${DESTDIR}/${BINDIR}/lprm
	install -s -g ${SPGRP} -m 2711 lpc ${DESTDIR}/usr/etc/lpc
	install -g ${SPGRP} -m 664 -c etc.printcap ${DESTDIR}/etc/printcap
	install -s pac ${DESTDIR}/usr/etc/pac
	cd filters && make ${MFLAGS} ${MRULES} install installv

clean:
	rm -f ${ALL} *.o
	cd filters; make ${MFLAGS} ${MRULES} clean

print:
	@pr makefile
	@${CXREF} *.c | pr -h XREF
	@pr *.h *.c

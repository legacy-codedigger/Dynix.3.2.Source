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

# $Header: Makefile 2.2 89/10/26 $
#
# make file for intermachine communications package
#
# Files are:
#	/etc/remote		remote host description file
#	/etc/phones		phone number file, owned by ${OWNER} and
#				  mode 6??
#	${ADM}/aculog		ACU accounting file, owned by ${OWNER} and
#				  mode 6?? {if ACULOG defined}
# Presently supports:
#	BIZCOMP
#	DEC DF02-AC, DF03-AC
#	DEC DN-11/Able Quadracall
#	VENTEL 212+
#	HAYES SMARTMODEM 1200
#       TELEBIT TRAILBLAZER PLUS (REV 4.0)
#	VADIC 831 RS232 adaptor
#	VADIC 3451
# (drivers are located in aculib.a)
#
# Configuration defines:
#	DF02, DF03, DN11	ACU's supported
#	  BIZ1031, BIZ1022, VENTEL, HAYES, V831, V3451, TELEBIT
#	ACULOG			turn on tip logging of ACU use
#	PRISTINE		no phone #'s put in ACU log file
#	CONNECT			worthless command
#	DEFBR			default baud rate to make connection at
#	DEFFS			default frame size for FTP buffering of
#				writes on local side
#	BUFSIZ			buffer sizing from stdio, must be fed
#				explicitly to remcap.c if not 1024
ADM=	usr/adm
OWNER=	uucp
GROUP=	daemon
CONFIG=	-DDF02 -DDF03 -DDN11 -DBIZ1031 -DBIZ1022 -DVENTEL -DHAYES -DV831 -DV3451 -DTELEBIT
CFLAGS=	-O -DDEFBR=1200 -DDEFFS=BUFSIZ -DACULOG ${INCLUDE}
SPECIAL=acutab.c remcap.c uucplock.c
DRIVERS=aculib/*.c
SOURCES=acu.c acutab.c cmds.c cmdtab.c cu.c hunt.c \
	log.c partab.c remote.c tip.c tipout.c value.c vars.c \
	${SPECIAL} ${DRIVERS}
CXREF=	/usr/ucb/ctags -xw
#

OBJS=	acu.o cmds.o cmdtab.o cu.o hunt.o \
	log.o partab.o remote.o tip.o tipout.o value.o vars.o

all:	tip

tip:&	${OBJS} acutab.o remcap.o uucplock.o aculib/aculib.a
	${CC} -o tip ${OBJS} acutab.o remcap.o uucplock.o \
		aculib/aculib.a

${OBJS}: tip.h

remcap.o: remcap.c
	${CC} -DBUFSIZ=1024 ${CFLAGS} -c remcap.c

# acutab is configuration dependent, and so depends on the makefile
acutab.o: Makefile
acutab.o: acutab.c
	${CC} ${CFLAGS} ${CONFIG} -c acutab.c

# remote.o depends on the makefile because of DEFBR and DEFFS
remote.o: Makefile

# log.o depends on the makefile because of ACULOG
log.o:	Makefile

aculib/aculib.a:  FRC
	cd aculib; make ${MFLAGS} ${MRULES}

FRC:

install:
	install -s -m 6711 -o ${OWNER} -g ${GROUP} tip ${DESTDIR}/usr/bin/tip
	rm -f ${DESTDIR}/usr/bin/cu
	ln ${DESTDIR}/usr/bin/tip ${DESTDIR}/usr/bin/cu
	cp /dev/null ${DESTDIR}/${ADM}/aculog
	-/etc/chown ${OWNER} ${DESTDIR}/${ADM}/aculog
	chmod 600 ${DESTDIR}/${ADM}/aculog

tags:	${SOURCES} tip.h
	ctags -w ${SOURCES} tip.h

clean:
	cd aculib; make ${MFLAGS} ${MRULES} clean
	rm -f tip cu *.o

print:
	@pr README TODO makefile
	@size *.o | pr -h SIZES
	@${CXREF} tip.h ${SOURCES} | pr -h XREF
	@ls -l | pr 
	@pr tip.h ${SOURCES}

lint:
	-lint -hbacvx ${CFLAGS} ${SOURCES}

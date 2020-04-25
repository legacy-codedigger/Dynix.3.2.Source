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
#	@(#)Makefile	2.7 (Berkeley) 6/18/88
#
# optional flags are: MEASURE TESTING DEBUG
#
CFLAGS=	-O -I. $(INCLUDE)
MACHINE=tahoe
SRCDS=	acksend.c candidate.c correct.c master.c networkdelta.c readmsg.c \
	slave.c timed.c
OBJDS=	acksend.o candidate.o correct.o master.o networkdelta.o readmsg.o \
	slave.o timed.o
SRCCS=	cmds.c cmdtab.c timedc.c
OBJCS=	cmds.o cmdtab.o timedc.o
COMMS=	byteorder.c measure.c cksum.tahoe.c
COMMO=	byteorder.o measure.o cksum.tahoe.o
#COMMS=	byteorder.c measure.c cksum.${MACHINE}.c
#COMMO=	byteorder.o measure.o cksum.${MACHINE}.o

all:& timed timedc

timed:&	${OBJDS} ${COMMO} 
	${CC} ${CFLAGS} ${OBJDS} ${COMMO} ${LIBS} -o $@

timedc:& ${OBJCS} ${COMMO}
	${CC} ${CFLAGS} ${OBJCS} ${COMMO} ${LIBS} -o $@

cksum.${MACHINE}.o:
	${CC} ${CFLAGS} -c $*.c

clean: FRC
	rm -f ${OBJDS} ${OBJCS} ${COMMO} core timed timedc

depend: FRC
	mkdep ${CFLAGS} ${SRCDS} ${SRCCS} ${COMMS}

install: FRC
	install -s -o root -g daemon -m 755 timed ${DESTDIR}/etc/timed
	install -s -o root -g daemon -m 4511 timedc ${DESTDIR}/etc/timedc

lint: FRC
	lint ${CFLAGS} ${SRCDS} ${COMMS}
	lint ${CFLAGS} ${SRCCS} ${COMMS}

tags: FRC
	ctags ${SRCDS} ${SRCCS} ${COMMS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


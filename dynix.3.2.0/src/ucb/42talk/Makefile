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

# $Header: Makefile 1.1 89/10/09 $
#
CFLAGS=	-O ${INCLUDE}
#

DOBJECT = talkd.o announce.o process.o table.o print.o
TOBJECT = talk.o get_names.o display.o io.o ctl.o init_disp.o\
	  msgs.o get_addrs.o ctl_transact.o invite.o look_up.o
DSOURCE = talkd.c announce.c process.c table.c print.c
TSOURCE = talk.c get_names.c display.c io.c ctl.c init_disp.c\
	  msgs.c get_addrs.c ctl_transact.c invite.c look_up.c
INC 	= talk.h ctl.h talk_ctl.h
SOURCE = ${DSOURCE} ${TSOURCE}

all:	42talk 42talkd

42talkd:&	${DOBJECT}
	${CC} -o 42talkd ${DOBJECT}

42talk:&	${TOBJECT}
	${CC} -o 42talk ${TOBJECT} -lcurses -ltermlib

${DOBJECT}: ctl.h
${TOBJECT}: talk.h ctl.h talk_ctl.h

install:
	install -s 42talk  ${DESTDIR}/usr/ucb/42talk
	install -s 42talkd ${DESTDIR}/usr/etc/42talkd

lint:
	lint ${DSOURCE}

clean:
	rm -f *.o 42talk 42talkd errs core a.out

tags:	${SOURCE} ${INC}
	ctags ${SOURCE} ${INC}

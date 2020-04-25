#
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
#
# $Header: Makefile 1.4 91/02/20 $
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
#	@(#)Makefile	1.17 (Berkeley) 11/14/89
#

#
# TERMCAP	Define this if your system is termcap based,
#		otherwise a terminfo based system is assumed.
#
# SRCRT		Includes code to allow you to specify source routes.
#		Format is:
#			[!]@hop1@hop2...[@|:]dst
#		Leading ! means strict source route.
#
# SYSV_TERMIO	Define this if you have System V termio structures,
#		This code works under Cray UNICOS.
#
# NO_CC_T	Define this if your termio.h file does not have
#		a typedef for cc_t.
#
# USE_TERMIO	Define this if you have the POSIX termios structures.
#		This code works under the BSD 4.4 terminal driver.
#
# KLUDGELINEMODE Define this to get the kludged up version of linemode
#		that was in 4.3BSD.  This is a good thing to have
#		around for talking to older systems.
#
# NOSTRNCASECMP	Define this if you do not have strncasecmp() in
#		your C libarary.
#
# strrchr=rindex Define this if you do not have strrchr() in your
#		C libaray (but you do have rindex...)
#
# HAS_IP_TOS	Define this if you have the setsockopt() option for
#		setting the IP Type of Service bit (IP_TOS) and you
#		have the gettosbyname() function.
#
# NEED_GETTOS	Define this if you have the setsockopt() option for
#		setting the IP Type Of Service bits, but you don't
#		have the gettosbyname() function.  If you define
#		this, you don't need to define HAS_IP_TOS.
#


#
# LIBS should be set to any libraries that need to be included,
# like the termcap or terminfo library.  LIBPATH is the paths
# to these libraries, for dependencies.
#


# 4.4bsd:
#	make -f Makefile.generic ${WHAT} \
#		LIBS="-ltermcap" \
#		LIBPATH="/lib/libc.a /usr/lib/libtermcap.a" \
#		DEST=${DESTDIR}/usr/bin \
#		DEFINES="-DTERMCAP -DSRCRT -DKLUDGELINEMODE -DUSE_TERMIO"
#
# 4.3bsd:
#	make -f Makefile.generic ${WHAT} \
#		LIBS="-ltermcap" \
#		LIBPATH="/lib/libc.a /usr/lib/libtermcap.a" \
#		DEST=${DESTDIR}/usr/bin \
#		DEFINES="-DTERMCAP -DSRCRT -DKLUDGELINEMODE"

# unicos5.0 unicos5.1:
#	make -f Makefile.generic ${WHAT} \
#		LIBS="-lcurses -lnet" \
#		LIBPATH="/lib/libc.a /usr/lib/libcurses.a" \
#		DEST=${DESTDIR}/usr/ucb \
#		DEFINES="-Dvfork=fork -Dsignal=sigset -DSRCRT \
#			-DKLUDGELINEMODE -DSYSV_TERMIO -DNO_CC_T"

# unicos6.0:
#	make -f Makefile.generic ${WHAT} \
#		LIBS="-lcurses -lnet" \
#		LIBPATH="/lib/libc.a /usr/lib/libcurses.a" \
#		DEST=${DESTDIR}/usr/ucb \
#		DEFINES="-Dvfork=fork -Dsignal=bsdsignal -DSRCRT \
#			-DKLUDGELINEMODE -DSYSV_TERMIO -DHAS_IP_TOS"

# sun3.5:
#	make -f Makefile.generic ${WHAT} \
#		LIBS=-ltermcap \
#		LIBPATH="/lib/libc.a /usr/lib/libtermcap.a" \
#		DEST=${DESTDIR}/usr/ucb \
#		DEFINES="-DTERMCAP -DSRCRT -DKLUDGELINEMODE -DNOSTRNCASECMP"
# sun4.0.3c sun4.0:
#	make -f Makefile.generic ${WHAT} \
#		LIBS=-ltermcap \
#		LIBPATH="/lib/libc.a /usr/lib/libtermcap.a" \
#		DEST=${DESTDIR}/usr/ucb \
#		DEFINES="-DFILIO_H -DTERMCAP -DUSE_TERMIO -DNO_CC_T \
#			-DSRCRT -DKLUDGELINEMODE"

# sun4.1:
#	make -f Makefile.generic ${WHAT} \
#		LIBS=-ltermcap \
#		LIBPATH="/lib/libc.a /usr/lib/libtermcap.a" \
#		DEST=${DESTDIR}/usr/ucb \
#		DEFINES="-DFILIO_H -DTERMCAP -DUSE_TERMIO -DSRCRT \
#			-DKLUDGELINEMODE"

# ultrix3.1:
#	make -f Makefile.generic ${WHAT} \
#		LIBS=-ltermcap \
#		LIBPATH="/lib/libc.a /usr/lib/libtermcap.a" \
#		DEST=${DESTDIR}/usr/ucb \
#		DEFINES="-DTERMCAP -DSRCRT -DKLUDGELINEMODE"

# dynix3.0.12 dynix:
LIBS=-ltermcap -lseq
LIBPATH=/lib/libc.a /usr/lib/libtermcap.a
DEST=${DESTDIR}/usr/ucb
DEFINES=-DTERMCAP -DSRCRT -DKLUDGELINEMODE -Dstrrchr=rindex -DNO_STRDUP

INCLUDES= -I..

SRCS=	commands.c main.c network.c ring.c \
	sys_bsd.c telnet.c terminal.c \
	tn3270.c utilities.c

CFLAGS=	-O ${INCLUDES} ${DEFINES} ${INCLUDE} 

ALLHC=	${SRCS} \
	defines.h externs.h fdset.h general.h \
	ring.h types.h

OBJS=	commands.o main.o network.o ring.o sys_bsd.o \
	telnet.o terminal.o tn3270.o utilities.o
# MAN=	telnet.0

#
# These next three lines are not needed in 4.4BSD
#
.SUFFIXES: .0 .1
.1.0:
	nroff -man -h $< > $@

all: telnet

telnet:&	${OBJS} ${LIBPATH}
	${CC} -o $@ ${CFLAGS} ${OBJS} ${LIBS}

clean: FRC
	rm -f ${OBJS} core errs l.errs telnet

cleandir: clean
	rm -f ${MAN} tags .depend

clist:	FRC ${SRCS}
	@for i in ${SRCS} ; \
		do (echo ${DIRPATH}$$i); done

hclist:	FRC ${ALLHC}
	@for i in ${ALLHC} ; \
		do (echo ${DIRPATH}$$i); done

depend: FRC ${SRCS}
	mkdep ${CFLAGS} `make clist`

install: FRC
	install -s telnet ${DESTDIR}/usr/ucb/telnet

lint: FRC ${SRCS}
	lint ${CFLAGS} `make clist`

tags: FRC ${SRCS}
	ctags `make hclist`

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

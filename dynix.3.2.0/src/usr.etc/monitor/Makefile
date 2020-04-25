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
#

# $Header: Makefile 2.2 87/04/10 $
#

# Makefile for monitor

SRCS= sslib.c monitor.c setup.c munge.c display.c text.c
OBJS= sslib.o monitor.o setup.o munge.o display.o text.o

CFLAGS= -O $(INCLUDE)

all		: monitor

monitor		: & ${OBJS}
		$(CC) ${OBJS} -o monitor -lseq -lcurses -ltermlib

lint		:
		$(LINT) $(INCLUDE) $(SRCS) -lcurses

install		:
		install -s -g daemon -m 2755 monitor ${DESTDIR}/usr/etc

clean clobber	:
		rm -f $(OBJS) monitor


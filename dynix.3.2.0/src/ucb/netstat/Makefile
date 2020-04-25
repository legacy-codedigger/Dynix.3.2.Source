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

# $Header: Makefile 2.2 90/05/26 $
#
CFLAGS=	-O $(INCLUDE) $(DEBUG)
#
OBJS=	host.o if.o inet.o main.o mbuf.o route.o unix.o

all	:  netstat
netstat	:& $(OBJS); $(CC) $(CFLAGS) -o netstat $(OBJS)
install	:; install -s -g daemon -m 2755 netstat $(DESTDIR)/usr/ucb/netstat
clean	:; rm -f $(OBJS)

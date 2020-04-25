# @(#)$Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# @(#)$Header: Makefile 1.14 86/08/05 $
#
# Be Careful, this is dependent on where include files come from!

CFLAGS= $(INCLUDE)

CFILES= host.c getargs.c getrun.c lock.c parse.c \
	sendfile.c signals.c usage.c remote.c packet.c io.c

OBJS = host.o getargs.o getrun.o lock.o parse.o \
	sendfile.o signals.o usage.o remote.o packet.o io.o

all:	host

host:&	$(OBJS)
	$(CC) -o host $(OBJS)

install:	all
	install -m 2755 -s host $(DESTDIR)/usr/bin

clean:
	rm -f *.o host

lint: ; lint $(INCLUDE) -lc $(CFILES)

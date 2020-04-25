# $Copyright: $
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
# $Header: Makefile 1.2 1991/07/03 23:27:07 $
#
# Make, install, and lint disk striping utilities.
# Assumes that stripe.h has already been installed 
# in /usr/include/sys/stripe.h.
#
#	@(#)Makefile	1.0	(Convex)	85/06/10
#
VERSION	= 0.0.0.1
OPTLEV	= -g
LOCCFLAGS = $(INCLUDE)
CFLAGS	= $(OPTLEV) $(LOCCFLAGS)

all:&	getst putst newst

getst.o:
	$(CC) $(STRIPE_INCLUDE) $(CFLAGS) -c getst.c

putst.o:
	$(CC) $(STRIPE_INCLUDE) $(CFLAGS) -c putst.c

newst.o:
	$(CC) $(STRIPE_INCLUDE) $(CFLAGS) -c newst.c

stripecap.o:
	$(CC) $(STRIPE_INCLUDE) $(CFLAGS) -c stripecap.c

getst: getst.o 
	$(CC) -o getst $(LDFLAGS) getst.o

putst:& putst.o stripecap.o
	$(CC) -o putst $(LDFLAGS) putst.o stripecap.o
	
newst:& newst.o stripecap.o
	$(CC) -o newst $(LDFLAGS) newst.o stripecap.o
	
lint:
	lint getst.c > getst.lint 2>&1
	lint putst.c stripecap.c > putst.lint 2>&1
	lint newst.c stripecap.c > newst.lint 2>&1

install: all
	install -s -m 755 getst ${STRIPE_DESTDIR}/etc && \
	install -s -m 700 putst newst ${STRIPE_DESTDIR}/etc && \
	make clean

clean:
	rm -f getst putst newst *.o getst.lint putst.lint newst.lint
	rm -f errs a.out core

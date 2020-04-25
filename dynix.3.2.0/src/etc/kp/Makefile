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

# $Header: Makefile 1.1 86/10/07 $
#
# Makefile for NMI based kernel profiling tools
#

LIBS= -lseq
CFLAGS = -O $(INCLUDE)
BINARIES= kpop kppr kpsnap kpstat
OBJECTS= kpop.o kppr.o kpsnap.o kpstat.o kp_sym.o

all		: objects binaries

objects		: & $(OBJECTS)

binaries	: & $(BINARIES)

kpop		: kpop.o
		$(CC) -o kpop kpop.o $(LIBS)

kppr		: kppr.o kp_sym.o
		$(CC) -o kppr kppr.o kp_sym.o $(LIBS)

kpsnap		: kpsnap.o
		$(CC) -o kpsnap kpsnap.o $(LIBS)

kpstat		: kpstat.o
		$(CC) -o kpstat kpstat.o $(LIBS)

lint		:
		$(LINT) -hbx $(CFLAGS) kpop.c
		$(LINT) -hbx $(CFLAGS) kppr.c kp_sym.c
		$(LINT) -hbx $(CFLAGS) kpsnap.c
		$(LINT) -hbx $(CFLAGS) kpstat.c

clean:
		-rm -f kpdc kpop kppr kpsnap kpstat *.o a.out core OUT

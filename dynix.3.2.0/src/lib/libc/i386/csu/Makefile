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

# $Header: Makefile 1.4 87/06/22 $
#
# C startup Makefile
#
#	crt0.o	Normal C run time startoff
#	crt0c.o ditto with copyright notice installed
#		(used by ccs tools)
#	mcrt0.o C startoff with profiling hooks for prof(1)
#	gcrt0.o C startoff with profiling hooks for gprof(1)
#
FPFLAGS	=
CFLAGS	= $(INCLUDE) $(FPFLAGS)
AFLAGS	= -u
#
OBJS	= crt0.o mcrt0.o gcrt0.o

all		:& $(OBJS)
install		:
		  install -m 644 crt0.o  $(DESTDIR)/lib/crt0.o
		  install -m 644 mcrt0.o  $(DESTDIR)/lib/mcrt0.o
		  install -m 644 gcrt0.o  $(DESTDIR)/usr/lib/gcrt0.o

crt0.o		: crt0.c crt0.sed copyright
		  $(CC) -SO $(CFLAGS) -DCRT0 -UCOPYRIGHT crt0.c | \
		  sed -f crt0.sed | $(AS) -o crt0.o && \
		  $(LD) -x -r crt0.o -o crt0.O && mv crt0.O crt0.o

# target called by the ccs tools makefile
crt0c.o		: crt0.c crt0.sed copyright
		  $(CC) -SO $(CFLAGS) -DCRT0 -DCOPYRIGHT crt0.c | \
		  sed -f crt0.sed | $(AS) -o crt0c.o && \
		  $(LD) -x -r crt0c.o -o crt0c.O && mv crt0c.O crt0c.o

mcrt0.o		: crt0.c crt0.sed copyright mon.c
		  $(CC) -SO $(CFLAGS) -DMCRT0 -DPROF -UCOPYRIGHT crt0.c | \
		  sed -f crt0.sed | $(AS) $(AFLAGS) -o mcrt0.o && \
		  $(LD) -x -r mcrt0.o -o mcrt0.O && mv mcrt0.O mcrt0.o

gcrt0.o		: crt0.c crt0.sed gmon.c gmon.h copyright
		  $(CC) -SO $(CFLAGS) -DMCRT0 -DGPROF -UCOPYRIGHT crt0.c | \
		  sed -f crt0.sed | $(AS) $(AFLAGS) -o gcrt0.o && \
		  $(LD) -x -r gcrt0.o -o gcrt0.O && mv gcrt0.O gcrt0.o

clean		:; rm -f $(OBJS) crt0c.o

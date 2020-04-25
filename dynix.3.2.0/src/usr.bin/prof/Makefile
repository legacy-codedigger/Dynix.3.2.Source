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

# $Header: Makefile 2.1 86/03/19 $
#
# If you don't want to plot, take out the -Dplot and the ref. to plot.a
PLOT 	= -lplot -Dplot
CFLAGS	= -O $(INCLUDE)

prof:	prof.o
	$(CC) -o prof prof.o $(PLOT)

prof.o: prof.c
	$(CC) -c $(CFLAGS) $(PLOT) prof.c

install:
	install -s prof $(DESTDIR)/usr/bin

clean :
	rm -f *.o prof

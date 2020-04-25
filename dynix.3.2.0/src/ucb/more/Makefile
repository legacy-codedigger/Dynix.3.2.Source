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

# $Header: Makefile 2.1 86/03/25 $
#
CFLAGS	= -O $(INCLUDE)

all		:  more more.help
more		:  more.c; $(CC) $(CFLAGS) -o more more.c -ltermcap
install		:; install -s more $(DESTDIR)/usr/ucb/more
		   rm -f $(DESTDIR)/usr/ucb/page
		   ln $(DESTDIR)/usr/ucb/more $(DESTDIR)/usr/ucb/page
		   install -c -m 644 more.help $(DESTDIR)/usr/lib/more.help
clean clobber	:; rm -f more more.o

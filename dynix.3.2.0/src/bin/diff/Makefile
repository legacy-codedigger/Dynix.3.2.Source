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

# $Header: Makefile 2.1 86/04/07 $
#
DIFF	= /bin/diff
DIFFH	= /usr/lib/diffh
PR	= /bin/pr
CFLAGS	= -O $(DEBUG) -DDIFF='"$(DIFF)"' -DDIFFH='"$(DIFFH)"' -DPR='"$(PR)"' $(INCLUDE)
OBJS	= diff.o diffdir.o diffreg.o

all		:  diff diffh
diff		:& $(OBJS)
		   $(CC) $(CFLAGS) -o diff $(OBJS)
diffh		:  diffh.o
		   $(CC) $(CFLAGS) -o diffh diffh.o
clean		:; rm -f diff diffh $(OBJS) diffh.o
install		:; install -s -m 755 diff $(DESTDIR)/bin/diff
		   install -s -m 755 diffh $(DESTDIR)/usr/lib/diffh

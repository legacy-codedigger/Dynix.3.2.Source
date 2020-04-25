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

# $Header: Makefile 2.3 91/03/21 $
#
CFLAGS	= -O $(DEBUG) $(INCLUDE)
OBJS	= main.o init.o subr.o gettytab.o get_date.o

all		:  getty
getty		:& $(OBJS)
		   $(CC) $(CFLAGS) -o getty $(OBJS) -lseq -lutil
install		:; install -s -m 750 getty $(DESTDIR)/etc
		   install -c -m 644 gettytab $(DESTDIR)/etc/gettytab
clean		:; rm -f getty $(OBJS)
main.o		:  gettytab.h
init.o		:  gettytab.h
subr.o		:  gettytab.h

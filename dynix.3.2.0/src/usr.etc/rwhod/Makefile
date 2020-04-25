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

# $Header: Makefile 2.3 89/07/13 $
#
CFLAGS	= -O $(DEBUG) $(INCLUDE)

all		:  rwhod
rwhod		:  rwhod.c
		   $(CC) $(CFLAGS) -o rwhod rwhod.c
install		:; install -s rwhod $(DESTDIR)/usr/etc/rwhod
clean		:; rm -f rwhod

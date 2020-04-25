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

# $Header: Makefile 2.2 87/01/06 $
#
# reorder gives an editor command for most common terminals
# (in reverse order from n'th to 1'st most commonly used)
# to move them to the front of termcap
#

all		:  termcap.obj
termcap.obj	:; ex - termcap.src < reorder
install		:; install -m 644 termcap.obj $(DESTDIR)/usr/etc/termcap
		   ln -s ../usr/etc/termcap $(DESTDIR)/etc/termcap
		   install -c -m 444 tabset/* $(DESTDIR)/usr/lib
clean		:; rm -f termcap.obj

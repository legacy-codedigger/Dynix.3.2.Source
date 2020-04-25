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

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: Makefile 1.1 91/04/04 $"

#	showprocs make file

CFLAGS = -O $(INCLUDE)
LIBS = -lseq

all:	showprocs

showprocs: showprocs.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o showprocs showprocs.c $(LIBS)

install: showprocs
	install -s -m 755 -o root -g root showprocs $(DESTDIR)/etc

clean:
	-rm -f showprocs.o

clobber: clean
	rm -f showprocs

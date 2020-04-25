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

# $Header: Makefile 2.2 91/03/13 $
#
# Makefile to create, compile, and install a utility which prints
# the version number and creation date of the DYNIX system.
#

all:	version

version: version.c
	$(CC) $(CFLAGS) $(INCLUDE) -o version version.c -lseq

install:
	install -s version $(DESTDIR)/etc/version

clean:
	rm -f version

FRC:

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
#
# $Header: Makefile 1.1 91/04/03 $
#

CFLAGS= -O ${INCLUDE}

all:& tdmp tprint trpt

trpt: trpt.c
	${CC} $(CFLAGS) -o trpt trpt.c -lseq

tdmp: tdmp.c
	${CC} $(CFLAGS) -o tdmp tdmp.c -lseq

tprint: tprint.c
	${CC} $(CFLAGS) -o tprint tprint.c -lseq

clean:
	rm -f core trpt tdmp tprint *.o tdmp.dmp tprint.print

install: all
	install -s -m 2755 -g daemon trpt $(DESTDIR)/usr/etc/trpt
	install -s -m 2755 -g daemon tdmp $(DESTDIR)/usr/etc/tdmp
	install -s -m 2755 -g daemon tprint $(DESTDIR)/usr/etc/tprint

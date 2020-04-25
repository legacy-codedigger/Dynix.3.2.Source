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

# $Header: Makefile 2.0 86/01/28 $
#
CFLAGS=	-O ${INCLUDE}
#

all:	diff3

diff3: 	diff3.o
	${CC} -o diff3 diff3.o

install:
	install -s diff3 ${DESTDIR}/usr/lib/diff3
	install -c diff3.sh ${DESTDIR}/usr/bin/diff3

clean:
	rm -f diff3.o diff3 core a.out

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

# $Header: Makefile 2.1 86/05/07 $

FILES=	vpq.sh vpr.sh vprint.sh vprm.sh vtroff.sh

all:
install:
	install -c vprint.sh ${DESTDIR}/usr/ucb/vprint
	install -c vtroff.sh ${DESTDIR}/usr/ucb/vtroff
	install -c vpq.sh ${DESTDIR}/usr/ucb/vpq
	install -c vpr.sh ${DESTDIR}/usr/ucb/vpr
	install -c vprm.sh ${DESTDIR}/usr/ucb/vprm

clean:
	rm -f errs

vprintall:
	csh /usr/ucb/vprint ${FILES}

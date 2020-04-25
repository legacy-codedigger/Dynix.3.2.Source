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
# Programs that live in subdirectories, and have makefiles of their own.
#
SUBDIR=	libc

all:&	${SUBDIR}

${SUBDIR}: FRC
	cd $@; make ${MFLAGS} ${MRULES} 

install:
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} install); done

clean:
	rm -f a.out core *.s *.o
	for i in ${SUBDIR}; do (cd $$i; make ${MFLAGS} ${MRULES} clean); done

FRC:

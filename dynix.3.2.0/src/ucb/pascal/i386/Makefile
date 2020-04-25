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

# $Header: Makefile 1.1 89/03/12 $

BIN =

# make sure P is reserved (defined to "&" on command line for parallel)
P=

SUBDIR = pc0 pc1 pxp utilities

SOURCE=	Makefile

LOOP=	for i in ${SUBDIR}; do\
		echo $$i:;\
		cd $$i;\
		[ -f Makefile ] || co Makefile;\
		make ${MFLAGS} ${MRULES} DESTDIR=${DESTDIR} P='$P' $@;\
		cd ..;\
	done

all clean depend install spotless tags: FRC
	${LOOP}

lint: FRC
	@echo "Too much CPU usage"

print: FRC
	@echo "You must be kidding. Too much output."

source: ${SOURCE}
	${LOOP}

${SOURCE}:
	co $@

FRC:

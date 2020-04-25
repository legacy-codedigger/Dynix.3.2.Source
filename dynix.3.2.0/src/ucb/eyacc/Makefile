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

FILES=	ey.h ey0.c ey1.c ey2.c ey3.c ey4.c ey5.c

OBJS=	ey0.o ey1.o ey2.o ey3.o ey4.o ey5.o

all:	eyacc

eyacc:	${OBJS}
	${CC} -o eyacc ${OBJS}

install:
	install -s eyacc ${DESTDIR}/usr/ucb/eyacc

clean:
	rm -f *.o eyacc core a.out errs

print:
	@ls -l | pr
	@pr ${FILES} makefile

ey1.o:	ey.h
ey2.o:	ey.h
ey3.o:	ey.h
ey4.o:	ey.h

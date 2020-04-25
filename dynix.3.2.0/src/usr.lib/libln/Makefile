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

# $Header: Makefile 2.1 87/04/28 $
#
CFLAGS=	-O ${INCLUDE}
#
OBJS=	allprint.o main.o reject.o yyless.o yywrap.o

all:	libln.a

libln.a:& ${OBJS}
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 ${OBJS}
	@-chgrp daemon ${OBJS} > /dev/null 2>&1
	@-chown  root ${OBJS} > /dev/null 2>&1
	${AR} cru libln.a ${OBJS}

install:
	rm -f ${DESTDIR}/usr/lib/libl.a
	install -m 644 libln.a ${DESTDIR}/usr/lib/libln.a
	${RANLIB} ${DESTDIR}/usr/lib/libln.a
	ln ${DESTDIR}/usr/lib/libln.a ${DESTDIR}/usr/lib/libl.a

clean:
	rm -f *.o libln.a core a.out errs

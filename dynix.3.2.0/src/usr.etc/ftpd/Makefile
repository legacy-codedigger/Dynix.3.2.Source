# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 2.5 1991/05/06 17:45:33 $
#
CFLAGS=	-O ${INCLUDE}
#
OBJS=	ftpcmd.o ftpd.o glob.o popen.o strtok.o logwtmp.o

all:	ftpd

ftpd:&	${OBJS}
	${CC} -o ftpd ${OBJS}

install:
	install -s ftpd ${DESTDIR}/usr/etc/ftpd

clean:
	rm -f ftpd *.o *.s errs core a.out t.? yacc.* y.tab.c ftpcmd.c

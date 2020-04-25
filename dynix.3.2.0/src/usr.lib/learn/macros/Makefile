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
# Make install will build the macros learn lesson files
#
CFLAGS= -O ${INCLUDE}
#
OBJS=
FILES=	macros.a

all:	${OBJS}

install:
	-if [ ! -d ${DESTDIR}/usr/lib/learn/macros ]; then \
		mkdir ${DESTDIR}/usr/lib/learn/macros;		\
		chmod 755 ${DESTDIR}/usr/lib/learn/macros;	\
		/etc/chown root    ${DESTDIR}/usr/lib/learn/macros;	\
		/bin/chgrp daemon  ${DESTDIR}/usr/lib/learn/macros;	\
	fi; exit 0
	@echo "Moving macros lessons"
	@cp ${FILES} ${OBJS} ${DESTDIR}/usr/lib/learn/macros
	@cd ${DESTDIR}/usr/lib/learn/macros; 			\
		${AR} x macros.a; 				\
		rm -f macros.a; 				\
		chmod 644 *

clean:
	rm -f core ${OBJS} errs

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
# Make install will build the files learn lesson files
#
CFLAGS= -O ${INCLUDE}
#
OBJS=
FILES=	files.a

all:	${OBJS}

install:
	-if [ ! -d ${DESTDIR}/usr/lib/learn/files ]; then \
		mkdir ${DESTDIR}/usr/lib/learn/files;		\
		chmod 755 ${DESTDIR}/usr/lib/learn/files;	\
		/etc/chown root    ${DESTDIR}/usr/lib/learn/files;	\
		/bin/chgrp daemon  ${DESTDIR}/usr/lib/learn/files;	\
	fi; exit 0
	@echo "Moving files lessons"
	@cp ${FILES} ${OBJS} ${DESTDIR}/usr/lib/learn/files
	@cd ${DESTDIR}/usr/lib/learn/files; 			\
		${AR} x files.a; 				\
		rm -f files.a; 					\
		chmod 644 *;					\
		chmod 755 Init

clean:
	rm -f core ${OBJS} errs

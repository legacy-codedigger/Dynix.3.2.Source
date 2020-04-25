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
# Make install will build the learn lesson libraries
# from this directory. 
#
LESSONS=C editor eqn files macros morefiles vi
FILES=	Linfo Xinfo
SUBDIRS=learn learn/log learn/bin

all:&	${LESSONS} ${FILES}

${LESSONS}: FRC
	cd $@; make ${MFLAGS} ${MRULES}

FRC:

install:
	-for i in ${SUBDIRS}; do \
		if [ ! -d ${DESTDIR}/usr/lib/$$i ]; then \
			mkdir ${DESTDIR}/usr/lib/$$i; 		\
			chmod 755 ${DESTDIR}/usr/lib/$$i; 	\
			/etc/chown root   ${DESTDIR}/usr/lib/$$i; 	\
			/bin/chgrp daemon ${DESTDIR}/usr/lib/$$i; 	\
		fi; \
	done; exit 0
	chmod 777 ${DESTDIR}/usr/lib/learn/log
	@for i in ${FILES}; do \
		echo copy $$i; \
		install -c -m 644 $$i ${DESTDIR}/usr/lib/learn/$$i; \
	done
	@for i in ${LESSONS}; do \
		echo install $$i; \
		(cd $$i; make ${MFLAGS} ${MRULES} install); \
	done

clean:
	@for i in ${LESSONS}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} clean); \
	done

# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.2 91/03/27 $

#
# bootpd   - BOOTP server daemon
#
# $Header: Makefile 1.2 91/03/27 $
#

# The next few lines may be uncommented and changed to alter the default
# filenames bootpd uses for its configuration and dump files.
#CONFFILE=-DCONFIG_FILE=\"/usr/etc/bootptab\"
#DUMPFILE=-DDUMP_FILE=\"/usr/etc/bootpd.dump\"
#FILESPECS=${CONFFILE} ${DUMPFILE}

# Users of SunOS 4.0 may have to add -DSUNOS40 to BOOTPDOPT below.
# Remove the -DVEND_CMU if you don't wish to support the "CMU vendor format"
# in addition to the RFC1048 format.

BOOTPDOPT=-DSYSLOG -DDEBUG -DVEND_CMU -DPRIVATE=static ${FILESPECS}

INSTALL=install
ETCDIR=/usr/etc
OBJS=bootpd.o readfile.o hash.o
CFLAGS=${BOOTPDOPT} ${INCLUDE}

all: bootpd

bootpd:& ${OBJS}
	sh newvers.sh
	${CC} ${CFLAGS} -o bootpd version.c ${OBJS}

bootpd.o: bootpd.c bootpd.h bootp.h
readfile.o: readfile.c bootpd.h bootp.h
hash.o: hash.c hash.h

system: install

install: all
	${INSTALL} -s bootpd ${DESTDIR}${ETCDIR}/bootpd

clean:
	@rm -f core bootpd *.BAK *.CKP *~ .emacs* *.o version.c

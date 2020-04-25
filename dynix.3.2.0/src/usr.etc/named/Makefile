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
#
# $Header: Makefile 1.7 91/02/20 $
#
# Copyright (c) 1987, 1988, 1990 The Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that the above copyright notice and this paragraph are
# duplicated in all such forms and that any documentation,
# advertising materials, and other materials related to such
# distribution and use acknowledge that the software was developed
# by the University of California, Berkeley.  The name of the
# University may not be used to endorse or promote products derived
# from this software without specific prior written permission.
# THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
# WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
#
#	@(#)Makefile.dist	5.2 (Berkeley) 8/7/90
#
DEFINES= -DDEBUG -DSTATS -DLAME_DELEGATION -DFWD_LOOP -DPID_FIX -DTRACEROOT
RES=
### -DALLOW_T_UNSPEC -Dmalloc=rt_malloc -Dfree=rt_free
### ALLOC=storage.o
CFLAGS=	-O ${DEFINES} ${INCLUDE}
LDFLAGS= ${RES} -lseq
LIBC=	/lib/libc.a
SRCS=	db_dump.c db_load.c db_lookup.c db_reload.c db_save.c db_update.c \
	ns_forw.c ns_init.c ns_main.c ns_maint.c ns_req.c ns_resp.c \
	ns_sort.c ns_stats.c db_glue.c named-xfer.c
HDRS=	db.h ns.h
OBJS=	db_dump.o db_load.o db_lookup.o db_reload.o db_save.o db_update.o \
	ns_forw.o ns_init.o ns_main.o ns_maint.o ns_req.o ns_resp.o \
	ns_sort.o ns_stats.o db_glue.o $(ALLOC)
XFEROBJ=   named-xfer.o db_glue.o
SUBDIR=	tools

all:& named named-xfer tools

named:&	${OBJS} ${LIBC}
		${CC} -o $@ ${OBJS} ${LDFLAGS}

named-xfer:&	${XFEROBJ} ${LIBC}
	${CC} -o $@ ${XFEROBJ} ${LDFLAGS}

tools:&	FRC
		cd tools; make ${MFLAGS} ${MRULES} RES=${RES}

clean: FRC
	rm -f ${XFEROBJ} ${OBJS} core named xfer version.o version.c
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} clean); \
	done

depend: FRC
	mkdep ${CFLAGS} ${SRCS}

install: FRC
	install -s named ${DESTDIR}/usr/etc/named
	install -s named-xfer ${DESTDIR}/usr/etc/named-xfer
	for i in ${SUBDIR}; do \
		(cd $$i; make ${MFLAGS} ${MRULES} install); \
	done

lint: FRC
	lint -Dlint ${CFLAGS} ${SRCS}

tags: FRC
	ctags -t ${SRCS} ${HDRS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

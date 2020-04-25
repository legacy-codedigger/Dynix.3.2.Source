#  $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

#ident	"$Header: Makefile 1.1 89/08/13 $"

#  $Log:	Makefile,v $

#
# Copyright (c) 1987 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
#	@(#)Makefile	4.7 (Berkeley) 4/6/88
#
CFLAGS=	-O $(INCLUDE)
RES=
LIBC=	/lib/libc.a
SRCS=	nsquery.c nstest.c
OBJS=	nsquery.o nstest.o

all: nsquery nstest nslookup

nsquery: ${LIBC}
	${CC} -o $@ ${CFLAGS} nsquery.c ${RES}

# test programs and utilities
nstest: ${LIBC}
	@echo "The resolver routines in libc must be compiled with the -DDEBUG"
	@echo "option or nstest will not have any output."
	${CC} -o $@ ${CFLAGS} nstest.c ${RES}

nslookup: FRC
	cd nslookup; make ${MFLAGS} ${MRULES} RES=${RES}

clean: FRC
	cd nslookup; make ${MFLAGS} clean
	rm -f ${OBJS} core nsquery nstest

depend: FRC
	cd nslookup; make ${MFLAGS} depend
	mkdep -p ${CFLAGS} ${SRCS}

install: FRC
	cd nslookup; make ${MFLAGS} DESTDIR=${DESTDIR} install

lint: FRC
	cd nslookup; make ${MFLAGS} lint
	lint ${CFLAGS} ${SRCS}

tags: FRC
	cd nslookup; make ${MFLAGS} tags
	ctags ${SRCS}

FRC:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.


# IF YOU PUT ANYTHING HERE IT WILL GO AWAY

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
#

# $Header: Makefile 1.4 1991/05/15 16:08:32 $

#
# Copyright (c) 1987 Regents of the University of California.
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
#	@(#)Makefile	5.5 (Berkeley) 6/29/88
#
LIBC=	/lib/libc.a
RDIST=	/usr/ucb/rdist
CFLAGS=	-O -DRDIST=\"${RDIST}\" $(INCLUDE) $(DEBUG)
SRCS=	docmd.c expand.c gram.y lookup.c main.c server.c
OBJS=	docmd.o expand.o gram.o lookup.o main.o server.o

all: rdist

rdist:& ${OBJS} ${LIBC}
	${CC} -o $@ ${CFLAGS} ${OBJS}

clean: FRC
	rm -f ${OBJS} core rdist yacc.* y.tab.c

install: FRC
	install -s -o root -g daemon -m 4751 rdist ${DESTDIR}${RDIST}

lint: FRC
	lint ${CFLAGS} ${SRCS}

tags: FRC
	ctags ${SRCS}

FRC:

depend: ${SRCS}
	/bin/grep '^#[  ]*include' ${SRCS} \
		| sed '/<.*>/d' \
		| sed '/y\.tab/d' \
		| sed 's/:[^"]*"\([^"]*\)".*/: \1/' \
		| sed 's/\.c/.o/' >>makedep
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	echo '$$r makedep' >>eddep
	echo 'w' >>eddep
	cp Makefile Makefile.bak
	ed - Makefile < eddep
	rm eddep makedep
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile
	echo '# see make depend above' >> Makefile

# DO NOT DELETE THIS LINE -- make depend uses it
docmd.o: defs.h
docmd.o: defs.h
expand.o: defs.h
gram.y: defs.h
lookup.o: defs.h
main.o: defs.h
server.o: defs.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above

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

# $Header: Makefile 1.2 91/03/04 $
#
#TYPES = -DVOID_SIGHANDLER
READLINE_DEFINES = -DVI_MODE
CFLAGS=	-O -I. ${INCLUDE} $(TYPES) $(READLINE_DEFINES)
#
OBJS=	readline.o history.o funmap.o keymaps.o
LIB =   libreadline.a

all:	$(LIB)

$(LIB):& ${OBJS}
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 ${OBJS}
	@-chgrp daemon ${OBJS} > /dev/null 2>&1
	@-chown  root ${OBJS} > /dev/null 2>&1
	${AR} cru $(LIB) ${OBJS}

install:
	install -m 644 $(LIB) ${DESTDIR}/usr/lib/$(LIB)
	${RANLIB} ${DESTDIR}/usr/lib/$(LIB)

clean:
	rm -f *.o $(LIB).a core a.out errs

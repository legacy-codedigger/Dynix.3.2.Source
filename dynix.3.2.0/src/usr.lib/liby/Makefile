#
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
#
# $Header: Makefile 1.3 91/02/12 $
#

CFLAGS	= -O ${INCLUDE}
OBJS	= libmai.o libzer.o

all:	liby.a

liby.a:& $(OBJS)
	@echo '	Fixing modes, groups, and owners on *.o'
	@chmod    644 $(OBJS)
	@-chgrp daemon $(OBJS) > /dev/null 2>&1
	@-chown  root $(OBJS) > /dev/null 2>&1
	@echo "	building $@"
	@$(AR) rc liby.a $(OBJS)

install:
	install -m 644 liby.a $(DESTDIR)/usr/lib/liby.a
	$(RANLIB) $(DESTDIR)/usr/lib/liby.a

clean clobber:
	rm -f *.o liby.a core

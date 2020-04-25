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

#
# Copyright (c) 1983 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2 (Berkeley) 6/18/85
#
CFLAGS= -O $(INCLUDE)
FILES=	main.c interactive.c restore.c dirs.c symtab.c tape.c utilities.c
OBJS=	main.o interactive.o restore.o dirs.o symtab.o utilities.o

all: objects restore rrestore
	@echo "etc/restore: C workaround in restore.c present."

objects:& $(OBJS) tape.o rtape.o dumprmt.o

restore: $(OBJS) tape.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o restore $(OBJS) tape.o

rrestore: $(OBJS) rtape.o dumprmt.o
	$(CC) $(LDFLAGS) -o rrestore $(OBJS) rtape.o dumprmt.o

rtape.o: tape.c
	rm -f rtape.c && ln -s tape.c rtape.c && \
	$(CC) $(CFLAGS) -c -DRRESTORE rtape.c; rm -f rtape.c

dumprmt.o: ../dump/dumprmt.c
	$(CC) $(CFLAGS) -c ../dump/dumprmt.c

install:
	install -s restore $(DESTDIR)/etc/restore
	install -s -m 4755 rrestore $(DESTDIR)/etc/rrestore

clean:
	rm -f restore rrestore *.o errs a.out core

lint:
	lint $(FILES)

print:
	@pr /usr/include/protocols/dumprestore.h restore.h $(FILES)

$(OBJS): restore.h

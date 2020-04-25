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
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)Makefile	5.2 (Berkeley) 6/18/85
#
#	dump.h			header file
#	dumpitime.c		reads /etc/ddate
#	dumpmain.c		driver
#	dumpoptr.c		operator interface
#	dumptape.c		handles the mag tape and opening/closing
#	dumptraverse.c		traverses the file system
#	unctime.c		undo ctime
#
#	DEBUG			use local directory to find ddate and dumpdates
#	TDEBUG			trace out the process forking
#
PSRCS = \
	/usr/include/protocols/dumprestore.h dump.h \
	dumpmain.c dumptraverse.c dumptape.c dumpoptr.c dumpitime.c unctime.c

SRCS =  \
	dump.h dumpitime.c \
	dumpmain.c dumpoptr.c dumprmt.c dumprtape.c \
	dumptraverse.c unctime.c

OBJS = dumpitime.o dumpmain.o dumpoptr.o \
	dumptape.o dumptraverse.o unctime.o

ROBJS = dumpitime.o dumprmain.o dumpoptr.o \
	dumprmt.o dumprtape.o dumptraverse.o unctime.o

CFLAGS = -O -DRAWIOPGRND $(INCLUDE) $(DEBUG)

all: dump rdump

rdump:&	$(ROBJS)
	$(CC) $(CFLAGS) $(ROBJS) -o rdump

dump:&	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o dump

install:
	install -s rdump dump $(DESTDIR)/etc

dumprmain.c:
	ln -s dumpmain.c dumprmain.c
dumprmain.o: dumprmain.c
	$(CC) -c -DRDUMP $(CFLAGS) dumprmain.c

dumprtape.c:
	ln -s dumptape.c dumprtape.c
dumprtape.o: dumprtape.c
	$(CC) -c -DRDUMP $(CFLAGS) dumprtape.c

clean:
	rm -f *.o rdump dump errs dumprmain.c dumprtape.c

lint:
	lint $(DFLAGS) $(SRCS)

print:
	pr $(PSRCS) | lpr

$(OBJS): dump.h

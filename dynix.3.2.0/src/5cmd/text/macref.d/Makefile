
DESTDIR	= $(ATT_DESTDIR)
YFLAGS	= -d
YACC	= $(ATT_YACC)
CFLAGS	= $(ATT_INCLUDE) -O
CC	= $(ATT_CC)
AR	= $(ATT_AR)
RANLIB	= $(ATT_RANLIB)

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

INSDIR = $(DESTDIR)/usr/bin
SFILES = 
FFILES = macref.c macrform.c macrstat.c macrtoc.c main.c match.c
FILES = macref.o macrform.o macrstat.o macrtoc.o main.o match.o

compile all:  macref
	:

macref:	$(FILES)
	$(CC) $(CFLAGS) -o macref $(FILES) 

install:
	install -c -m 755 macref $(INSDIR)/macref

clean:
	rm -f $(FILES)
	rm -f macref

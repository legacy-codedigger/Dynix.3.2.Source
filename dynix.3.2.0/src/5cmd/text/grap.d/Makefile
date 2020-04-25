
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

# ALLOC = malloc.o
OFILES = main.o input.o print.o frame.o for.o coord.o ticks.o plot.o label.o misc.o $(ALLOC)
CFILES = main.c input.c print.c frame.c for.c coord.c ticks.c plot.c label.c misc.c
SRCFILES = grap.y grapl.l grap.h $(CFILES)
INSDIR = $(DESTDIR)/usr/bin
LIBDIR = $(DESTDIR)/usr/lib/dwb

all:	grap

grap:	grap.o grapl.o $(OFILES) grap.h 
	$(CC) -o grap $(CFLAGS) grap.o grapl.o $(OFILES) -lm

$(OFILES) grapl.o:	grap.h prevy.tab.h

grap.o:	grap.h

y.tab.h:	grap.o

prevy.tab.h:	y.tab.h
	-cmp -s y.tab.h prevy.tab.h || cp y.tab.h prevy.tab.h

install:
	install -c -m 755 grap $(INSDIR)/grap
	install -c -m 644 grap.defines $(LIBDIR)/grap.defines

clean clobber:
	rm -f grap.o grapl.o $(OFILES) y.tab.h prevy.tab.h grap

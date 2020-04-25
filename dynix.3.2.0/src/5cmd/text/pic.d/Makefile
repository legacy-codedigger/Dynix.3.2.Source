
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

OFILES = main.o print.o misc.o symtab.o blockgen.o boxgen.o circgen.o \
	arcgen.o linegen.o movegen.o textgen.o \
	input.o for.o pltroff.o
CFILES = main.c print.c misc.c symtab.c blockgen.c boxgen.c circgen.c \
	arcgen.c linegen.c movegen.c textgen.c \
	input.c for.c pltroff.c
SRCFILES = picy.y picl.l pic.h $(CFILES)
INSDIR = $(DESTDIR)/usr/bin

all:	pic

pic::	picy.o picl.o $(OFILES)
	$(CC) -o pic $(CFLAGS) picy.o picl.o $(OFILES) -lm

$(OFILES):	pic.h
picy.c:	picy.y pic.h
picl.c:	picl.l pic.h

y.tab.h:	picy.o

pic.ydef:	y.tab.h
	-cmp -s y.tab.h pic.ydef || cp y.tab.h pic.ydef

pltroff:	driver.o pltroff.o
	$(CC)  $(CFLAGS) -o pltroff pltroff.o driver.o -lm

install:
	install -c -m 755 pic $(INSDIR)/pic

clean clobber:
	rm -f $(OFILES) picy.o picl.o y.tab.h picy.c picl.c pic pltroff

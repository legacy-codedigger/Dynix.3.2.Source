
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
SOURCE = chekl.l  chekmain.c chekrout.c
FILES = chekl.o chekmain.o chekrout.o
SOURCE1 = chekl1.l chekmain1.c chekrout1.c
FILES1 = chekl1.o chekmain1.o chekrout1.o

compile all: checkmm1 checkmm
	:

checkmm1:	$(FILES1)
	 $(CC) -s $(CFLAGS) -o checkmm1 $(FILES1) -ll -lPW

checkmm:	$(FILES)
	$(CC) -s  $(CFLAGS) -o checkmm $(FILES) -ll -lPW

install:
	install -c -m 755 checkmm $(INSDIR)/checkmm
	install -c -m 755 checkmm1 $(INSDIR)/checkmm1

clean clobber:
	  rm -f $(FILES) $(FILES1)
	  rm -f checkmm checkmm1

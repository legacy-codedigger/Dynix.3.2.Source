
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
DIRS= checkmm eqn grap macref macros neqn pic ptx roff shells subndx tbl

compile all:&	 $(DIRS) diffmk hyphen
	:

$(DIRS):
	cd $@.d;  make $(MFLAGS) $(MRULES)

diffmk:	diffmk.sh
	cp diffmk.sh diffmk

hyphen:	hyphen.c
	$(CC) $(CFLAGS) hyphen.c -o hyphen

install:
	install -c -m 775 diffmk $(INSDIR)/diffmk
	install -c -m 775 hyphen $(INSDIR)/hyphen
	-for i in $(DIRS); do \
		cd $$i.d; make $(MFLAGS) $(MRULES) install; cd ..; \
	done

clean clobber:
	for i in $(DIRS); do \
		cd $$i.d; make $(MFLAGS) $(MRULES) clean; cd ..; \
	done
	rm -f hyphen.o hyphen diffmk



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
INSLIB = $(DESTDIR)/usr/lib
HINSDIR = $(DESTDIR)/usr/pub
FILES =  mm.sh mmt.sh 
SFILES = nroff.letter mm.report mm.sales mm.letter tbl.language tbl.bridges \
	tbl.pres eqn.stats troff.fonts troff.sizes troff.ad \
	troff.aeneid pic.forms

compile all:  mm mmt mvt
	:

install:
	install -c -m 755 mm $(INSDIR)/mm
	install -c -m 755 mmt $(INSDIR)/mmt
	rm -f $(INSDIR)/mvt
	ln $(INSDIR)/mmt $(INSDIR)/mvt
	install -c -m 664 terminals $(HINSDIR)/terminals
	-for i in $(SFILES); do \
		install -c -m 644 $$i $(INSLIB)/dwb/samples/$$i; \
	done

mm:	mm.sh
	cp mm.sh mm

mmt:	mmt.sh
	cp mmt.sh mmt

mvt:	mmt
	:

clean clobber:
	-rm -f mm mmt

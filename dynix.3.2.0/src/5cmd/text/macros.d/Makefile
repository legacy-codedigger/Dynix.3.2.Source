
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

MINSLIB = $(DESTDIR)/usr/lib/macros
TINSLIB = $(DESTDIR)/usr/lib/tmac
TMACFILES = tmac.an tmac.m tmac.ptx tmac.v

compile all:	mmn mmt vmca ptx man
	:

mmn:	mmn.src strings.mm
	sh ./macrunch mmn

strings.mm:	strings.mm.src
	sh ./macrunch strings.mm

mmt:	mmt.src strings.mm
	sh ./macrunch mmt

vmca:	vmca.src
	sh ./macrunch vmca

man:	an.src
	sh ./macrunch an

ptx:	ptx.src
	sh ./macrunch ptx

install:
	install -c -m 644 mmn $(MINSLIB)/mmn
	install -c -m 644 strings.mm $(MINSLIB)/strings.mm
	install -c -m 644 mmt $(MINSLIB)/mmt
	install -c -m 644 vmca $(MINSLIB)/vmca
	install -c -m 644 an $(MINSLIB)/an
	install -c -m 644 ptx $(MINSLIB)/ptx
	-for i in $(TMACFILES); do \
		install -c -m 644 $$i $(TINSLIB)/$$i; \
	done

clean clobber:
	-rm -f mmn mmt vmca ptx an strings.mm

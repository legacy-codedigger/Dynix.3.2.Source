
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

ARGS = all
INSDIR = $(DESTDIR)/usr/bin
INSLIB = $(DESTDIR)/usr/lib

compile all:	subj ndx

subj:	sbj1 sbj2 sbj3 parts
	cp subj.sh subj

sbj1:	sbj1.o cnst.h
	$(CC) $(CFLAGS) sbj1.o -ll -o sbj1

sbj2:	sbj2.o case.o cnst.h
	$(CC) $(CFLAGS) sbj2.o case.o -ll -o sbj2

sbj3:	sbj3.o case.o omit.o cnst.h
	$(CC) $(CFLAGS) sbj3.o case.o omit.o -ll -o sbj3

ndx:	ndexer pages ndxformat sbjprep
	cp ndx.sh ndx

ndexer:	ndexer.o rootwd.o str.o strr.o case.o space.o dstructs.h ehash.h edict.h
	$(CC) $(CFLAGS) ndexer.o rootwd.o str.o strr.o case.o space.o -ll -i -o ndexer

pages:	pages.c
	$(CC) $(CFLAGS) pages.c -o pages

ndxformat:	ndxformat.c
		$(CC) $(CFLAGS) ndxformat.c -o ndxformat

sbjprep:	sbjprep.c
		$(CC) $(CFLAGS) sbjprep.c -o sbjprep

parts:	parts.sh style1 style2 style3 deroff
	cp parts.sh parts

style1:	nwords.o nhash.h dict.h ydict.h names.h abbrev.h
	$(CC) $(CFLAGS) nwords.o -ll -o style1

style2:	end.o ehash.h edict.h names.h
	$(CC) $(CFLAGS) end.o -ll -o style2

style3:	part.o pscan.o outp.o extern.o
	$(CC) $(CFLAGS) part.o pscan.o outp.o extern.o -ll -o style3

deroff:	deroff.o
	$(CC) $(CFLAGS) deroff.o -i -o deroff

install:
	install -c -m 775 subj $(INSDIR)/subj
	install -c -m 755 sbj1 $(INSLIB)/dwb/sbj1
	install -c -m 755 sbj2 $(INSLIB)/dwb/sbj2
	install -c -m 755 sbj3 $(INSLIB)/dwb/sbj3
	install -c -m 775 ndx $(INSDIR)/ndx
	install -c -m 755 ndexer $(INSLIB)/dwb/ndexer
	install -c -m 755 pages $(INSLIB)/dwb/pages
	install -c -m 755 ndxformat $(INSLIB)/dwb/ndxformat
	install -c -m 755 sbjprep $(INSLIB)/dwb/sbjprep
	install -c -m 755 parts $(INSLIB)/dwb/parts
	install -c -m 755 style1 $(INSLIB)/dwb/style1
	install -c -m 755 style2 $(INSLIB)/dwb/style2
	install -c -m 755 style3 $(INSLIB)/dwb/style3
	install -c -m 755 deroff $(INSLIB)/dwb/deroff

clean clobber:
	rm -f sbj1.o sbj2.o sbj3.o case.o omit.o end.o nwords.o part.o \					pscan.o outp.o extern.o deroff.o
	rm -f ndexer.o rootwd.o str.o strr.o case.o space.o pages.o \					 ndxformat.o sbjprep.o
	rm -f sbj1 sbj2 sbj3 subj parts style1 style2 style3 deroff
	rm -f ndx ndexer pages ndxformat sbjprep

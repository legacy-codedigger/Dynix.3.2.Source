
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

RASTDIR = $(DESTDIR)/usr/lib/font/devi10/rasti10
FILES = [A-Z].[0-9]* [A-Z][A-Z0-9].[0-9]*
3BRASTDIR = 3b-rast

all:	i10_rasts aps

i10_rasts:
	-sh -c 'for i in $(FILES); do \
		if [ ! -s $$i ]; then \
			echo ERROR: bad raster file $$i; \
		fi; \
	done; exit 0'

make3brast:	make3brast.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o make3brast make3brast.c

aps:
	cd aps-i10; make $(MRULES) $(MFLAGS)

fdump:	fdump.c ../glyph.h
	$(CC) $(CFLAGS) -o fdump fdump.c

fbuild:	fbuild.c ../glyph.h
	$(CC) $(CFLAGS) -o fbuild fbuild.c

install:
	for i in $(FILES); do \
		install -c -m 644 $$i $(RASTDIR)/$$i; \
	done
	install -c -m 644 RASTERLIST $(RASTDIR)/RASTERLIST
	cd aps-i10; make $(MRULES) $(MFLAGS) install

clean clobber:
	rm -f *.o
	rm -f make3brast fdump fbuild
	rm -rf $(3BRASTDIR)
	cd aps-i10;  make $(MRULES) $(MFLAGS) clobber

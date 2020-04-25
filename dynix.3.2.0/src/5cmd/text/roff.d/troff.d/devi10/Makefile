
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

# to use the draw.c that take advantage of the Imagen graphics primitives,
#	change the definition of "DRAW" in the next line to "impdraw"
#	N.B  This will only work with Imagen versions 1.9 or later
DRAW = impdraw

INSDIR = $(DESTDIR)/usr/bin
FONTHOME = $(DESTDIR)/usr/lib/font
FONTDIR = $(DESTDIR)/usr/lib/font/devi10
MAKEDEV = ../makedev
FFILES = [A-Z] [A-Z][0-9A-Z] DESC
OFILES = [A-Z].out [A-Z][0-9A-Z].out DESC.out
UNSUP = buildrast makefonts makei10 printrast readrast

all:	di10 i10_fonts i10_rasts

di10 dimpress:	dimpress.o $(DRAW).o glob.o misc.o rast.o oldrast.o
	$(CC) $(CFLAGS) -o $@ dimpress.o $(DRAW).o \
		glob.o misc.o rast.o oldrast.o -lm

dimpress.o:	ext.h gen.h ../dev.h impcodes.h dimpress.h spectab.h
	if [ "$(DRAW)" = "impdraw" ] ; then \
		$(CC) $(CFLAGS) -DIMPDRAW -I../ -c dimpress.c ; \
	else \
		$(CC) $(CFLAGS) -I../ -c dimpress.c ; \
	fi
glob.o:	gen.h init.h
misc.o:	ext.h gen.h
rast.o:	ext.h gen.h rast.h
readrast.o:	ext.h gen.h rast.h
printrast.o:	ext.h gen.h rast.h impcodes.h
buildrast.o:	ext.h gen.h init.h rast.h buildrast.h
editrast.o:	gen.h rast.h buildrast.h editrast.h
oldrast.o:	ext.h gen.h glyph.h impcodes.h dimpress.h
impdraw.o:	gen.h impcodes.h

../draw.o:	../draw.c
	cd ..;  make $(MFLAGS) $(MRULES) draw.o


i10_fonts:	$(MAKEDEV)
	$(MAKEDEV) DESC
	for i in $(FFILES); do \
		$(MAKEDEV) $$i; \
	done
	-if [ -r LINKFILE ]; then \
	    sh ./LINKFILE; \
	fi

$(MAKEDEV):	$(MAKEDEV).c ../dev.h
	$(CC) $(CFLAGS) -I../ $(LDFLAGS) -o $(MAKEDEV) $(MAKEDEV).c

i10_rasts:	$(MAKEDEV)
	cd rasti10; make $(MRULES) $(MFLAGS)

unsup:	$(UNSUP)

makei10:	makei10.c ../dev.h
	$(CC) $(CFLAGS) -I../ -o makei10 makei10.c
makefonts:	makefonts.c ../dev.h
	$(CC) $(CFLAGS) -I../ -o makefonts makefonts.c

readrast:	readrast.o glob.o misc.o rast.o
	$(CC) $(CFLAGS) -o readrast readrast.o glob.o misc.o rast.o
printrast:	printrast.o glob.o misc.o rast.o
	$(CC) $(CFLAGS) -o printrast printrast.o glob.o misc.o rast.o
buildrast:	buildrast.o glob.o misc.o rast.o editrast.o
	$(CC) $(CFLAGS) -o buildrast buildrast.o glob.o misc.o rast.o editrast.o

install:
	install -c -m 775 di10 $(INSDIR)/di10
	for i in $(OFILES); do \
		install -c -m 644 $$i $(FONTDIR)/$$i; \
	done
	cd rasti10; make $(MRULES) $(MFLAGS) install

clean clobber:
	rm -f *.o
	rm -f $(OFILES) di10 dimpress $(UNSUP)
	cd rasti10;  make $(MRULES) $(MFLAGS) clobber

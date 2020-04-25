
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

FONTDIR = $(DESTDIR)/usr/lib/font/devi10/rasti10/devaps
MAKEDEV = ../../../makedev
FFILES = [A-Z] [A-Z][0-9A-Z] DESC
OFILES = [A-Z].out [A-Z][0-9A-Z].out DESC.out

all:	aps_fonts

aps_fonts:	$(MAKEDEV)
	$(MAKEDEV) DESC
	for i in $(FFILES); do \
		$(MAKEDEV) $$i; \
	done
	-if [ -r LINKFILE ]; then \
	    sh ./LINKFILE; \
	fi

$(MAKEDEV):	$(MAKEDEV).c
	$(CC) $(CFLAGS) -o $(MAKEDEV) $(MAKEDEV).c

install:
	for i in $(OFILES); do \
		install -c -m 644 $$i $(FONTDIR)/$$i; \
	done

clean clobber:
	rm -f *.o
	rm -f *.out


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

INSLIB = $(DESTDIR)/usr/lib
INSDIR = $(DESTDIR)/usr/bin
FILES = ptx.c eign.sh
LDFLAGS = -s

all: ptx eign

ptx:	ptx.c
	$(CC) $(CFLAGS) $(LDFLAGS) ptx.c -o ptx

eign:	eign.sh
	cp eign.sh eign

install:
	install -c -m 755 ptx $(INSDIR)/ptx
	install -c -m 644 eign $(INSLIB)/eign

clean clobber:
	rm -f *.o
	rm -f eign ptx

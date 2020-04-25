
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

INCORE = -DINCORE

compile all:&  nroff terms troff fonts

nroff:
	cd nroff.d;   make $(MFLAGS) $(MRULES) INCORE=$(INCORE) nroff

troff:
	cd troff.d;   make $(MFLAGS) $(MRULES) INCORE=$(INCORE) troff

terms:
	cd nroff.d;  make $(MFLAGS) $(MRULES) terms
fonts:
	cd troff.d;  make $(MFLAGS) $(MRULES) fonts

install:
	cd nroff.d; make $(MFLAGS) $(MRULES) install
	cd troff.d; make $(MFLAGS) $(MRULES) install

clean clobber:
	cd nroff.d;  make $(MFLAGS) $(MRULES) clean
	cd troff.d;  make $(MFLAGS) $(MRULES) clean

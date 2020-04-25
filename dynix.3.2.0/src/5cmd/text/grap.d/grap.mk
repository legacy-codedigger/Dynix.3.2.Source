#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)grap:grap.mk	1.5"
#	makefile for grap.
#


OL = $(ROOT)/
# ALLOC = malloc.o
YFLAGS = -d -D
OFILES = main.o input.o print.o frame.o for.o coord.o ticks.o plot.o label.o misc.o $(ALLOC)
CFILES = main.c input.c print.c frame.c for.c coord.c ticks.c plot.c label.c misc.c
SRCFILES = grap.y grapl.l grap.h $(CFILES)
INS = :
INSDIR = $(OL)usr/bin
LIBDIR = $(OL)usr/lib/dwb
IFLAG = -i
LDFLAGS = -s

all:	$(LIBDIR)/grap.defines grap

grap:	grap.o grapl.o $(OFILES) grap.h 
	$(CC) -o grap $(IFLAG) $(FFLAG) $(LDFLAGS) grap.o grapl.o $(OFILES) -lm
	$(INS) grap $(INSDIR)
	cd $(INSDIR); chmod 755 grap; $(CH) chgrp bin grap; chown bin grap

$(LIBDIR)/grap.defines:	grap.defines
	if [ ! -d $(LIBDIR) ]; then rm -f $(LIBDIR); mkdir $(LIBDIR); \
		chmod 755 $(LIBDIR);  fi
	$(INS) grap.defines $(LIBDIR)
	cd $(LIBDIR); chmod 644 grap.defines;
	$(CH) chgrp bin grap.defines; chown bin grap.defines

$(OFILES) grapl.o:	grap.h prevy.tab.h

grap.o:	grap.h

y.tab.h:	grap.o

prevy.tab.h:	y.tab.h
	-cmp -s y.tab.h prevy.tab.h || cp y.tab.h prevy.tab.h

install:
	$(MAKE) -f grap.mk INS=cp ROOT=$(ROOT) CH=$(CH)

clean:
	rm -f grap.o grapl.o $(OFILES) y.tab.h prevy.tab.h

clobber:	clean
	rm -f grap

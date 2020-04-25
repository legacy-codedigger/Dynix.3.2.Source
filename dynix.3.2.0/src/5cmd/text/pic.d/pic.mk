#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	makefile for pic.
#
# DSL 2.
# SCCS @(#)pic:pic.mk	1.9


OL = $(ROOT)/
YFLAGS = -d
OFILES = main.o print.o misc.o symtab.o blockgen.o boxgen.o circgen.o \
	arcgen.o linegen.o movegen.o textgen.o \
	input.o for.o pltroff.o
CFILES = main.c print.c misc.c symtab.c blockgen.c boxgen.c circgen.c \
	arcgen.c linegen.c movegen.c textgen.c \
	input.c for.c pltroff.c
SRCFILES = picy.y picl.l pic.h $(CFILES)
INS = :
INSDIR = $(OL)usr/bin
IFLAG = -i
LDFLAGS = -s

all:	pic

pic::	picy.o picl.o $(OFILES)
	$(CC) -o pic $(IFLAG) $(FFLAG) $(LDFLAGS) picy.o picl.o $(OFILES) -lm
	$(INS) pic $(INSDIR)
	cd $(INSDIR); chmod 755 pic; $(CH) chgrp bin pic; chown bin pic

$(OFILES):	pic.h
picy.c:	picy.y pic.h
picl.c:	picl.l pic.h

y.tab.h:	picy.o

pic.ydef:	y.tab.h
	-cmp -s y.tab.h pic.ydef || cp y.tab.h pic.ydef

pltroff:	driver.o pltroff.o
	$(CC)  $(IFLAG) -o pltroff pltroff.o driver.o -lm

install:
	$(MAKE) -f pic.mk INS=cp ROOT=$(ROOT) CH=$(CH)

clean:
	rm -f $(OFILES) picy.o picl.o y.tab.h picy.c picl.c

clobber:	clean
	rm -f pic pltroff

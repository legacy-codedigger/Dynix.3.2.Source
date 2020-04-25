#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)macref:macref.mk	1.8"
#	text subsystem macref make file
#
# DSL 2

OL = $(ROOT)/
INS = :
INSDIR = $(OL)usr/bin
IFLAG = -n
LDFLAGS = -s
SFILES = 
FFILES = macref.c macrform.c macrstat.c macrtoc.c main.c match.c
FILES = macref.o macrform.o macrstat.o macrtoc.o main.o match.o
MAKE = make

compile all:  macref
	:

macref:	$(FILES)
	$(CC) $(LDFLAGS) $(IFLAG) -o macref $(FILES) 
	$(INS) macref $(INSDIR)
	cd $(INSDIR); chmod 755 macref; $(CH) chgrp bin macref; chown bin macref



install:
	$(MAKE) -f macref.mk INS=cp ROOT=$(ROOT) CH=$(CH)

clean:
	rm -f $(FILES)
clobber:  clean 
		rm -f macref

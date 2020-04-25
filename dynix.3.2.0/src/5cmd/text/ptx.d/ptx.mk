#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ptx:ptx.mk	1.9"
#
#

OL = $(ROOT)/
INS = :
INSLIB = $(ROOT)/usr/lib
INSDIR = $(ROOT)/usr/bin
FILES = ptx.c eign.sh
LDFLAGS = -s
MAKE = make

all: ptx eign

ptx:	ptx.c
	$(CC) -O $(LDFLAGS) ptx.c -o ptx
	$(INS) ptx $(INSDIR)
	cd $(INSDIR); chmod 775 ptx; $(CH) chgrp bin ptx; chown bin ptx

eign:	eign.sh
	cp eign.sh eign
	$(INS) eign $(INSLIB)
	cd $(INSLIB); chmod 644 eign; $(CH) chgrp bin eign; chown bin eign

install:
	$(MAKE) -f ptx.mk INS=cp ROOT=$(ROOT) CH=$(CH)
insptx:	;  $(MAKE) -f ptx.mk INS=cp ROOT=$(ROOT) CH=$(CH) ptx
inseign:	;  $(MAKE) -f ptx.mk INS=cp ROOT=$(ROOT) CH=$(CH) eign

clean:
	rm -f *.o

clobber: clean
	rm  -f eign ptx

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)roff:roff.mk	2.6"
#	nroff/troff make file (text subsystem)
#
# DSL 2

CFLAGS = -O
LDFLAGS = -s
INS = :
MAKE = make
INCORE = -DINCORE

compile all:  nroff terms troff fonts

nroff:
	- if u3b2 || u3b5 || u3b15 ; \
	then cd nroff.d;   $(MAKE) -f nroff.mk nroff INS=$(INS) ROOT=$(ROOT) \
		INCORE= CH=$(CH) CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) ; fi
	- if vax || u3b ; \
	then cd nroff.d;   $(MAKE) -f nroff.mk nroff INS=$(INS) ROOT=$(ROOT) \
		INCORE=$(INCORE) CH=$(CH) CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) ; fi

troff:
	- if u3b2 || u3b5 || u3b15 ; \
	then cd troff.d;   $(MAKE) -f troff.mk troff INS=$(INS) ROOT=$(ROOT) \
		INCORE= CH=$(CH) CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) ; fi
	- if vax || u3b ; \
	then cd troff.d;   $(MAKE) -f troff.mk troff INS=$(INS) ROOT=$(ROOT) \
		INCORE=$(INCORE) CH=$(CH) CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) ; fi

terms:
	cd nroff.d;  $(MAKE) -f nroff.mk terms INS=$(INS) ROOT=$(ROOT) CH=$(CH)
fonts:
	cd troff.d;  $(MAKE) -f troff.mk fonts INS=$(INS) ROOT=$(ROOT) CH=$(CH)

install:
	$(MAKE) -f roff.mk INS=cp ROOT=$(ROOT) CH=$(CH) INCORE=$(INCORE) all \
		CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS)
insnroff:
	$(MAKE) -f roff.mk INS=cp ROOT=$(ROOT) CH=$(CH) INCORE=$(INCORE) nroff
instroff:
	$(MAKE) -f roff.mk INS=cp ROOT=$(ROOT) CH=$(CH) INCORE=$(INCORE) troff

clean:
	cd nroff.d;  $(MAKE) -f nroff.mk clean
	cd troff.d;  $(MAKE) -f troff.mk clean

clobber:
	cd nroff.d;  $(MAKE) -f nroff.mk clobber
	cd troff.d;  $(MAKE) -f troff.mk clobber

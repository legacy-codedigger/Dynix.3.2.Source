#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)eqn:eqn.mk	2.11"
#	eqn make file (text subsystem)
#
# for DSL 2

OL = $(ROOT)/
INS = :
INSDIR = $(OL)usr/bin
PINSDIR = $(OL)usr/pub
CFLAGS = -O
LDFLAGS = -s
YFLAGS = -d
SOURCE = e.y e.h main.c diacrit.c eqnbox.c font.c fromto.c funny.c \
glob.c integral.c io.c lex.c lookup.c mark.c matrix.c move.c over.c paren.c \
 pile.c shift.c size.c sqrt.c text.c

OFILES =  main.o diacrit.o eqnbox.o font.o fromto.o funny.o \
glob.o integral.o io.o lex.o lookup.o mark.o matrix.o move.o over.o paren.o \
 pile.o shift.o size.o sqrt.o text.o
FILES =  $(OFILES) e.o
MAKE = make

compile all: eqn apseqnch cateqnch eqnch
	:

eqn:	$(FILES)
	$(CC) $(FFLAG) $(LDFLAGS) -o eqn $(FILES) -ly
	$(INS) eqn $(INSDIR)
	cd $(INSDIR); chmod 755 eqn; $(CH) chgrp bin eqn; chown bin eqn

$(OFILES): e.h e.def

e.o:	e.h

e.def:	  y.tab.h
	  -cmp -s y.tab.h e.def || cp y.tab.h e.def

y.tab.h:  e.o
	:

apseqnch:	apseqnchar
	$(INS) apseqnchar $(PINSDIR)
	cd $(PINSDIR); chmod 644 apseqnchar; $(CH) chgrp bin apseqnchar; chown bin apseqnchar

cateqnch:	cateqnchar
	$(INS) cateqnchar $(PINSDIR)
	cd $(PINSDIR); chmod 644 cateqnchar; $(CH) chgrp bin cateqnchar; chown bin cateqnchar

eqnch:	apseqnch cateqnch
	if [ $(INS) != : ]; \
	then \
		rm -f $(PINSDIR)/eqnchar; \
		ln $(PINSDIR)/apseqnchar $(PINSDIR)/eqnchar; \
	fi

install:
	$(MAKE) -f eqn.mk INS=cp ROOT=$(ROOT) CH=$(CH)
inseqnch:
	$(MAKE) -f eqn.mk INS=cp ROOT=$(ROOT) CH=$(CH) eqnch

clean:
	  rm -f *.o y.tab.h e.def

clobber:  clean
	  rm -f eqn

#	Copyright (c) 1984 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)neqn:neqn.mk	1.21"
#	neqn make file (text subsystem)
#
# DSL 2

OL = $(ROOT)/
INS = :
INSDIR = $(OL)usr/bin
CFLAGS = -O -DNEQN
LDFLAGS = -s
YFLAGS = -d
SOURCE = e.y e.h diacrit.c eqnbox.c font.c fromto.c funny.c glob.c integral.c \
	 io.c lex.c lookup.c mark.c matrix.c move.c over.c paren.c \
	 pile.c shift.c size.c sqrt.c text.c
OFILES =  diacrit.o eqnbox.o font.o fromto.o funny.o glob.o integral.o \
	 io.o lex.o lookup.o mark.o matrix.o move.o over.o paren.o \
	 pile.o shift.o size.o sqrt.o text.o
FILES = $(OFILES) e.o
MAKE = make

compile all:	neqn
	:

neqn:	$(FILES)
	$(CC) $(LDFLAGS) $(IFLAG) $(FFLAG) -o neqn $(FILES) -ly
	$(INS) neqn $(INSDIR)
	cd $(INSDIR); chmod 755 neqn; $(CH) chgrp bin neqn; chown bin neqn

$(OFILES):: e.h e.def

e.def:    y.tab.h
	  -cmp -s y.tab.h e.def || cp y.tab.h e.def

y.tab.h:  e.o

install:
	$(MAKE) -f neqn.mk INS=cp ROOT=$(ROOT) CH=$(CH)

clean:
	rm -f *.o y.tab.[ch] e.def

clobber:  clean
	rm -f neqn

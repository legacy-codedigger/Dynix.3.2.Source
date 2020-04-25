# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.1 89/03/12 $

DEST = $(DESTDIR)/usr/lib
BINDIR = ../pc0
SRCDIR = ../src

WHOAMI = pc
VERSION = 3.1
DATE = 7/14/86
MKSTR = /usr/ucb/mkstr
EYACC = /usr/ucb/eyacc

ERRORSTRINGS = $(WHOAMI)$(VERSION)strings
HOWFILE = how_$(WHOAMI)

DEBUG= -g
CFLAGS = ${DEBUG} -I. -I$(SRCDIR) $(INCLUDE)
LDFLAGS = -z

HDR = $(SRCDIR)/0.h $(SRCDIR)/OPnames.h $(SRCDIR)/align.h $(SRCDIR)/iorec.h \
      $(SRCDIR)/objfmt.h $(SRCDIR)/pstab.h $(SRCDIR)/pc.h $(SRCDIR)/tree.h \
      $(SRCDIR)/tree_ty.h $(SRCDIR)/tmps.h $(SRCDIR)/yy.h $(SRCDIR)/config.h \
      whoami.h

SRC = $(SRCDIR)/ato.c $(SRCDIR)/call.c $(SRCDIR)/clas.c $(SRCDIR)/const.c \
      $(SRCDIR)/conv.c $(SRCDIR)/cset.c $(SRCDIR)/error.c $(SRCDIR)/fdec.c \
      $(SRCDIR)/fend.c $(SRCDIR)/fhdr.c $(SRCDIR)/flvalue.c $(SRCDIR)/forop.c \
      $(SRCDIR)/hash.c $(SRCDIR)/lab.c $(SRCDIR)/lookup.c $(SRCDIR)/lval.c \
      $(SRCDIR)/stklval.c $(SRCDIR)/main.c $(SRCDIR)/nl.c $(SRCDIR)/put.c \
      $(SRCDIR)/rec.c $(SRCDIR)/rval.c $(SRCDIR)/stkrval.c $(SRCDIR)/stat.c \
      $(SRCDIR)/string.c $(SRCDIR)/subr.c $(SRCDIR)/tmps.c $(SRCDIR)/tree.c \
      $(SRCDIR)/type.c $(SRCDIR)/var.c $(SRCDIR)/TRdata.c $(SRCDIR)/treen.c \
      $(SRCDIR)/yycopy.c $(SRCDIR)/yycosts.c $(SRCDIR)/yyerror.c \
      $(SRCDIR)/yyget.c $(SRCDIR)/yyid.c $(SRCDIR)/yylex.c $(SRCDIR)/yymain.c \
      $(SRCDIR)/yyoptions.c $(SRCDIR)/yypanic.c $(SRCDIR)/yyparse.c \
      $(SRCDIR)/yyprint.c $(SRCDIR)/yyput.c $(SRCDIR)/yyrecover.c \
      $(SRCDIR)/yyseman.c $(SRCDIR)/yytree.c $(SRCDIR)/p2put.c \
      $(SRCDIR)/stab.c $(SRCDIR)/pcproc.c $(SRCDIR)/pcfunc.c \
      $(SRCDIR)/pccaseop.c $(SRCDIR)/pclval.c $(SRCDIR)/sconv.c

TSRC = $(SRCDIR)/pas.y $(SRCDIR)/opc.c $(SRCDIR)/CONFIG.c $(SRCDIR)/gram \
       $(SRCDIR)/pic.c

OBJ = ato.o call.o clas.o const.o conv.o cset.o error.o fdec.o fend.o \
      fhdr.o flvalue.o forop.o hash.o lab.o lookup.o lval.o stklval.o main.o \
      nl.o put.o rec.o rval.o stkrval.o stat.o string.o subr.o tmps.o tree.o \
      type.o var.o TRdata.o treen.o yycopy.o yycosts.o yyerror.o yyget.o \
      yyid.o yylex.o yymain.o yyoptions.o yypanic.o yyparse.o yyprint.o \
      yyput.o yyrecover.o yyseman.o yytree.o p2put.o stab.o pcproc.o pcfunc.o \
      pccaseop.o pclval.o sconv.o


all:	pc0

pc0: 	opcode.h y.tab.o $(OBJ) config.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o pc0 $(OBJ) y.tab.o config.c

$(OBJ):
	cd $(SRCDIR) && \
	    $(MKSTR) - $(BINDIR)/$(ERRORSTRINGS) $(BINDIR)/ `basename $@ .o`.c
	$(CC) $(CFLAGS) -c `basename $@ .o`.c

y.tab.h y.tab.c: $(SRCDIR)/pas.y $(SRCDIR)/gram
	rm -f y.tab.h y.tab.c
	$(EYACC) $(SRCDIR)/pas.y > /dev/null
	ex - y.tab.c < $(SRCDIR)/gram
	@echo "	***That makes y.tab.h and y.tab.c***"

y.tab.o: y.tab.c y.tab.h
	rm -f x.y.tab.c
	$(MKSTR) - $(ERRORSTRINGS) x. y.tab.c
	$(CC) $(CFLAGS) -c x.y.tab.c
	mv x.y.tab.o y.tab.o
	rm -f x.y.tab.c

picture: $(SRCDIR)/OPnames.h $(SRCDIR)/pic.c
	rm -f picture
	cc $(SRCDIR)/pic.c -o pic
	./pic > picture
	rm -f pic

opcode.h: $(SRCDIR)/OPnames.h $(SRCDIR)/opc.c
	rm -f opcode.h
	cc $(SRCDIR)/opc.c -o opc
	./opc > opcode.h
	rm -f opc

$(SRCDIR)/0.h:
	touch -f $(SRCDIR)/0.h

config.c: $(SRCDIR)/CONFIG.c Makefile
	sed -e "s?VERSION?$(VERSION)?g" \
	    -e "s?DATE?$(DATE)?g" \
	    -e "s?INSTALLDIR?/usr/lib?g" \
	    -e "s?LIBDIR?/usr/lib?g" \
	    -e "s?ERRORSTRINGS?${ERRORSTRINGS}?g" \
	    -e "s?HOWFILE?${HOWFILE}?g" \
	    < $(SRCDIR)/CONFIG.c > config.c
clean: FRC
	rm -f *.o *.c y.tab.h y.tab.c y.tab.out $(WHOAMI)*strings config.c \
	      opcode.h picture *.list *.bak a.out core tmp yacc.acts yacc.tmp \
	      pc0 errs opc pic tags

install:
	install -c -m 444 $(ERRORSTRINGS) $(DESTDIR)/usr/lib/$(ERRORSTRINGS)
	install -s pc0 $(DESTDIR)/usr/lib/pc0

FRC:

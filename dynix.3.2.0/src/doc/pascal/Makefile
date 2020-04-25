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
#
# Copyright (c) 1980 Regents of the University of California.
# All rights reserved.  The Berkeley software License Agreement
# specifies the terms and conditions for redistribution.
#
#	@(#)makefile	6.1 (Berkeley) 5/7/86
#
# This makefile assumes that csh is running the figures

# $Header: Makefile 1.3 87/04/10 $

C	= n
O	= out
TPFLAGS	= ../liberal
STAGE	= $(STAGEROOT)/vol2/pascal
INSTDIR	= $(DESTDIR)/usr/doc/pascal
LPR	= lpr -Pimagen -n
IMAGEN	= /usr/imagen
IBIN	= $(IMAGEN)/bin
TPF	= $(IBIN)/itpf -X
TMACTPF = $(IMAC)/tmac.tpf

.SUFFIXES:
.SUFFIXES: .n .out .p .F .P .Y

FIGURES	=\
	first.$O\
	bigger.$O bigger4.$O bigger7.$O\
	primes.$O\
	digits.$O synerr.$O synerr2.$O\
	mism.$O clash.$O sin1.$O sin2.$O\
	expr.$O typequ.$O primes2.$O\
	kat.$O copydata.$O kat2.$O primes2.$O

figures	:& $(FIGURES)

figprint:
	O=P figures

SRC	= puman0.$C puman1.$C puman2.$C puman3.$C puman4.$C puman5.$C pumanA.$C

source	:& $(SRC)

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

# NOTE: the following target DOES NOT WORK.
#	I can't seem to get the stderr redirection to work.
#	I've been doing this by hand.

.p.out	:
#	- pc $< -o $* 2>&1 $@
#	$* >> $@
#	rm $*

.out.P:
	imprint $<

install	:; install -c -m 444 $(FIGURES) $(SRC) $(INSTDIR)
#	install -c -m 444 Makefile $(INSTDIR)/Makefile

# "make file.P" prints file.F.
.F.P :;\
	$(LPR) $<

# Files with the .t suffix use tpf macros.
.n.F :;\
	$(TPF) -M$(TMACTPF) $(TPFLAGS) $< > $(STAGE)/$@;\
	rm -f $@;\
	ln -s $(STAGE)/$@

# Files with the .t suffix use tpf macros.
.n.Y :;\
	$(ITPF) -M$(TMACTPF) $<

format	:&;
	C=F source

mkstage	:; -mkdir $(STAGE)

print	: format;
	C=P source

clean	:; rm -f *.F $(STAGE)/*.F

final	:&;
	C=Y source

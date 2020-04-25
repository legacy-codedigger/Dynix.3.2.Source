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

# $Header: Makefile 1.14 90/09/05 $
#
TPFLAGS	= ../man.macs
IMAGEN	= /usr/imagen
IBIN	= $(IMAGEN)/bin
ITPF	= $(IBIN)/itpf -x
YTPF	= $(IBIN)/itpf -y
TMAC	= /usr/lib/tmac
TMACMS	= $(TMAC)/tmac.s

INSTDIR	= $(DESTDIR)/usr/doc/csh

.SUFFIXES:
.SUFFIXES: .1 .2 .3 .4 .a .g .1P .2P .3P .4P .aP .gP .1Y .2Y .3Y .4Y .aY .gY

SRC	= csh.1 csh.2 csh.3 csh.4 csh.a csh.g tabs

PRINT	= csh.1P csh.2P csh.3P csh.4P csh.aP csh.gP

FINAL	= csh.1Y csh.2Y csh.3Y csh.4Y csh.aY csh.gY

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile

.1.1P .2.2P .3.3P .4.4P .a.aP .g.gP:
	$(ITPF) -M$(TMACMS) $(TPFLAGS) $<

.1.1Y .2.2Y .3.3Y .4.4Y .a.aY .g.gY:
	$(YTPF) -M$(TMACMS) $(TPFLAGS) $<

print	:& $(PRINT)

final	:& $(FINAL)

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

# $Header: Makefile 1.22 87/04/15 $
#
TPFLAGS	= ../man.macs p.mac
IMAGEN	= /usr/imagen
IBIN	= $(IMAGEN)/bin
ITPF	= $(IBIN)/itpf -x
YTPF	= $(IBIN)/itpf -y
TMAC	= /usr/lib/tmac
TMACMSB	= $(TMAC)/tmac.sbell

REFER	= refer -e -p Rv7man

INSTDIR	= $(DESTDIR)/usr/doc/cacm

SRC	= p1 p2 p3 p4 p5 p6

ALL	= $(SRC) Rv7man Rv7man.ig p.mac

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install		:; install -c -m 444 $(ALL) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile

# Note: the dashes at the end of the following two target are
#	necessary for itpf to know it's getting standard input

print	:;
	cat $(SRC) | $(REFER) | $(ITPF) -M$(TMACMSB) $(TPFLAGS) -

final	:;
	cat $(SRC) | $(REFER) | $(YTPF) -M$(TMACMSB) $(TPFLAGS) -

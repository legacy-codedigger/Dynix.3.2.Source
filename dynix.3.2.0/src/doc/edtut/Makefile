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

# $Header: Makefile 1.12 87/04/13 $
#
TPFLAGS	= ../man.macs e.mac
IMAGEN	= /usr/imagen
IBIN	= $(IMAGEN)/bin
ITPF	= $(IBIN)/itpf -x
YTPF	= $(IBIN)/itpf -y
TMAC	= /usr/lib/tmac
TMACMSB	= $(TMAC)/tmac.sbell

INSTDIR	= $(DESTDIR)/usr/doc/edtut

SRC	= e.mac e0 e1 e2 e3 e4 e5 e6 e7

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile

print	:; $(ITPF) -M$(TMACMSB) $(TPFLAGS) $(SRC)

final	:; $(YTPF) -M$(TMACMSB) $(TPFLAGS) $(SRC)

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

# $Header: Makefile 1.11 87/04/14 $
#
TPFLAGS	= ../man.macs ../picmacs
IMAGEN	= /usr/imagen
IBIN	= $(IMAGEN)/bin
ITPF	= $(IBIN)/itpf -x
YTPF	= $(IBIN)/itpf -y
TMAC	= /usr/lib/tmac
TMACME	= $(TMAC)/tmac.e

INSTDIR	= $(DESTDIR)/usr/doc/gprof

SRC	= \
	header.me abstract.me intro.me profiling.me gathering.me\
	postp.me present.me refs.me


default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile

print	:; $(ITPF) -M$(TMACME) $(TPFLAGS) $(SRC)

final	:; $(YTPF) -M$(TMACME) $(TPFLAGS) $(SRC)

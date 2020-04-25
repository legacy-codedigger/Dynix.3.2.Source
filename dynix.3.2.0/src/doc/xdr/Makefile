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
# $Header: Makefile 1.2 87/07/27 $
#
C	= t
TPFLAGS	= ../man.macs
ITPF	= /usr/imagen/bin/itpf -x
YTPF	= /usr/imagen/bin/itpf -y
TMACTPF	= /usr/imagen/lib/tmac/tmac.tpf

CCSMAKE	= /ccsbin/ccsmake TOOLS=/ccs/techpubs/tools

INSTDIR	= $(DESTDIR)/usr/doc/xdr

SRC	= rpc.program.$C rpc.spec.$C xdr.spec.$C

source	:& $(SRC)

.SUFFIXES:
.SUFFIXES: .t .P .Y

.t.P	:;
	$(ITPF) -M$(TMACTPF) $(TPFLAGS) $<

.t.Y	:;
	$(YTPF) -M$(TMACTPF) $(TPFLAGS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:; install -c -m 444 $(SRC) $(INSTDIR)
#	install -c -m 444 Makefile $(INSTDIR)/Makefile

print	:; $(CCSMAKE) C=P source

final	:; $(CCSMAKE) C=Y source

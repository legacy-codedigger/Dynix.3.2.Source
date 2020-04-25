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

# $Header: Makefile 1.18 87/04/13 $
#
C	= nr
TPFLAGS	= ../man.macs
INSTDIR	= $(DESTDIR)/usr/doc/Mail
IMAGEN	= /usr/imagen
IBIN	= $(IMAGEN)/bin
ITPF	= $(IBIN)/itpf -x
YTPF	= $(IBIN)/itpf -y
TMAC	= /usr/lib/tmac
TMACME	= $(TMAC)/tmac.e

SRC	= \
	mail0.$C mail1.$C mail2.$C mail3.$C mail4.$C mail5.$C \
	mail6.$C mail7.$C mail8.$C mail9.$C maila.$C

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile

# The source is printed together because the page numbering depends on it

print	:;
	$(ITPF) -M$(TMACME) $(TPFLAGS) $(SRC)

final	:;
	$(YTPF) -M$(TMACME) $(TPFLAGS) $(SRC)

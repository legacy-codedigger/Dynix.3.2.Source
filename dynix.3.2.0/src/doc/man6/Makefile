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

# $Header: Makefile 1.29 87/07/29 $
#
.SUFFIXES:
.SUFFIXES: .6 .Z .P

M	= $(DESTDIR)/usr/man/cat6
C	= 6
ITPF	= /usr/imagen/bin/itpf -x

# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

PAGES	=\
	arithmetic.$C backgammon.$C banner.$C bcd.$C boggle.$C canfield.$C\
	cribbage.$C factor.$C fish.$C fortune.$C hangman.$C number.$C\
	quiz.$C rain.$C rogue.$C sail.$C snake.$C trek.$C worm.$C worms.$C\
	wump.$C

.6.Z	:
	cd ..; umask 111; nroff $(MANMACS) man6/$< | compress > $M/$<.Z
.6.P	:
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:; install -c -m 444 $(PAGES) $(DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z pages
	install    -m 444 factor.6.eqn $(DESTDIR)/usr/man/man$C/factor.6
	install -c -m 444 factor.6 $(DESTDIR)/usr/man/man$C/factor.6.eqn
	touch $M/factor.6.Z

pages	:& $(PAGES)

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages

# Special (for eqn processing)
factor.Z:
	cd ..; soelim < man6/$*.6 | neqn > man6/$*.6.eqn ; \
	umask 111; nroff $(MANMACS) man6/$*.6.eqn | compress > $M/$*.6.Z

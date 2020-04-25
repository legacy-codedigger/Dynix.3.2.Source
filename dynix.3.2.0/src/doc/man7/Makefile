# $Copyright: $
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.20 1991/08/27 22:47:25 $
#
M	= $(DESTDIR)/usr/man/cat7
C	= 7
ITPF	= /usr/imagen/bin/itpf -x

# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

PAGES	=\
	ascii.$C environ.$C eqnchar.$C hier.$C hostname.$C\
	intro.$C mailaddr.$C man.$C me.$C ms.$C term.$C

.SUFFIXES:
.SUFFIXES: .7 .Z .P

.7.Z	:
	cd ..; umask 111; nroff $(MANMACS) man7/$< | compress > $M/$<.Z

.7.P	:
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:; install -c -m 444 $(PAGES) $(DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z pages

pages	: & $(PAGES)

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages

# Specials (for tbl processing)
mailaddr.Z:
	cd ..; tbl < man7/mailaddr.7 > man7/mailaddr.7.tbl ; \
	umask 111; nroff $(MANMACS) man7/mailaddr.7.tbl | compress > $M/mailaddr.7.Z

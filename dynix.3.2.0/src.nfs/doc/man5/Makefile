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

# $Header: Makefile 1.8 87/07/29 $
#
.SUFFIXES:
.SUFFIXES: .5 .Z .P

M	= $(NFS_DESTDIR)/usr/man/cat5
C	= 5
ITPF	= /usr/imagen/bin/itpf -x

IPATH	= $(DESTDIR)/usr/include
# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

SED	= sed -e '/Copyright/,/^$$/ d' -e '/Log/,/^$$/ d' \
	-e 's/[$$]Header/$$\\\&Header/'

PAGES	=\
	exports.$C ypfiles.$C

.5.Z	:
	cd ..; umask 111; nroff $(MANMACS) man5/$< | compress > $M/$<.Z
.5.P	:
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:; install -c -m 444 $(PAGES) $(NFS_DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) MANMACS="$(MANMACS)" C=Z pages

pages	:& $(PAGES)

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages

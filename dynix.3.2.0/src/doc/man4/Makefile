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

# $Header: Makefile 1.46 1991/07/03 21:50:53 $
#
.SUFFIXES:
.SUFFIXES: .4 .4p .4f .4n .Z .Zp .Zf .Zn .P .Pp .Pf .Pn

M	= $(DESTDIR)/usr/man/cat4
C	= 4
ITPF	= /usr/imagen/bin/itpf -x

# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

PAGES	=\
	appletalk.$Cf arp.$Cp autoconf.$C cons.$C ddp.$Cp drum.$C gate.$C\
	icmp.$Cp if.$Cn inet.$Cf intro.$C intro.$Cn ip.$Cp lo.$C lp.$C mem.$C\
	mtio.$C null.$C pmap.$C pty.$C sd.$C se.$C smem.$C ssm.$C st.$C\
	tcp.$Cp tm.$C tg.$C ts.$C tty.$C udp.$Cp usclk.$C xp.$C xt.$C\
	wd.$C zd.$C

STRIPE_PGS = ds.$C

MIRROR_PGS = mr.$C

.4.Z .4p.Zp .4f.Zf .4n.Zn:
	cd ..; umask 111; nroff $(MANMACS) man4/$< | compress > $M/$<.Z

.4.P .4p.Pp .4f.Pf .4n.Pn:
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	: mirror_install stripe_install
	install -c -m 444 $(PAGES) $(DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z pages

stripe_install :
	install -c -m 444 $(STRIPE_PGS) $(STRIPE_DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z M=$(STRIPE_DESTDIR)/usr/man/cat$C/	\
		stripe_pages
	
mirror_install :
	install -c -m 444 $(MIRROR_PGS) $(MIRROR_DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z M=$(MIRROR_DESTDIR)/usr/man/cat$C/	\
		mirror_pages
	
pages	: & $(PAGES)

stripe_pages	:&	$(STRIPE_PGS)

mirror_pages	:&	$(MIRROR_PGS)

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages
stripe_print	:
	 /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P stripe_pages
mirror_print	:
	 /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P mirror_pages

# Special (for tbl processing)
lp.Z st.Z:
	cd ..; tbl < man4/$*.4 > man4/$*.4.tbl ; \
	umask 111; nroff $(MANMACS) man4/$*.4.tbl | compress > $M/$*.4.Z

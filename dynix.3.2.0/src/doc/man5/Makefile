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

# $Header: Makefile 1.42 1992/02/12 22:14:16 $
#
.SUFFIXES:
.SUFFIXES: .5 .Z .P

M	= $(DESTDIR)/usr/man/cat5
C	= 5
ITPF	= /usr/imagen/bin/itpf -x
IPATH	= $(DESTDIR)/usr/include

# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

#No longer need this line-  SOS	= acct.5 ar.5 types.5 utmp.5

SED	= sed -e '/Copyright/,/^$$/ d' -e '/Log/,/^$$/ d' \
	-e 's/[$$]Header/$$\\\&Header/'

PAGES	=\
	L-devices.$C L-dialcodes.$C L.aliases.$C L.cmds.$C L.sys.$C\
	USERFILE.$C a.out.$C acct.$C aliases.$C ar.$C core.$C dir.$C\
	disktab.$C diskinfo.$C \
	dump.$C ethers.$C fs.$C fstab.$C gettytab.$C group.$C hosts.$C\
	hosts.lpd.$C hosts.equiv.$C mntent.$C mtab.$C netgroup.$C networks.$C\
	passwd.$C phones.$C plot.$C printcap.$C protocols.$C rcsfile.$C\
	remote.$C resolver.$C rmtab.$C rpc.$C sdb.$C servers.$C\
	services.$C shells.$C stab.$C\
	tar.$C termcap.$C tp.$C ttys.$C ttytype.$C types.$C\
	utmp.$C uuencode.$C vfont.$C vgrindefs.$C vtoc.$C

STRIPE_PGS = stripecap.$C

MIRROR_PGS = mrtab.$C

.5.Z	:
	cd ..; umask 111; nroff $(MANMACS) man5/$< | compress > $M/$<.Z
.5.P	:
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

# Specials (for tbl processing)
diskinfo.Z: diskinfo.5
	umask 111; tbl diskinfo.5 | nroff -i $(MANMACS) | compress >$M/diskinfo.5.Z

install	:& $(SOS) mirror_install stripe_install
	install -c -m 444 $(PAGES) $(DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) MANMACS="$(MANMACS)" C=Z pages

stripe_install:
	install -c -m 444 $(STRIPE_PGS) $(STRIPE_DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z M=$(STRIPE_DESTDIR)/usr/man/cat$C/	\
		stripe_pages
	
mirror_install:
	install -c -m 444 $(MIRROR_PGS) $(MIRROR_DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z M=$(MIRROR_DESTDIR)/usr/man/cat$C/	\
		mirror_pages
	
pages	:& $(PAGES)
stripe_pages	:&	$(STRIPE_PGS)
mirror_pages	:&	$(MIRROR_PGS)

#Special handling for pages that .so a header file
#acct.5	: acct.5.so
#	$(SED) < $(IPATH)/sys/acct.h > acct.h && soelim acct.5.so > $@
#ar.5	: ar.5.so
#	$(SED) < $(IPATH)/ar.h > ar.h && soelim ar.5.so > $@
#types.5	: types.5.so
#	$(SED) < $(IPATH)/sys/types.h > types.h && soelim types.5.so > $@
#utmp.5	: utmp.5.so
#	$(SED) < $(IPATH)/utmp.h > utmp.h && soelim utmp.5.so > $@

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages
stripe_print	:
	 /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P stripe_pages
mirror_print	:
	 /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P mirror_pages

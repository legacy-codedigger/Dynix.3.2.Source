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

# $Header: Makefile 1.79 1991/08/13 17:24:07 $
#
.SUFFIXES:
.SUFFIXES: .8 .8c .8s .Z .Zc .Zs .P .Pc .Ps

M	= $(DESTDIR)/usr/man/cat8
C	= 8
ITPF	= /usr/imagen/bin/itpf -x
# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

PAGES	=\
	ac.$C addbad.$C adduser.$C arcv.$C arp.$Cc bad144.$C badsect.$C\
	boot.$Cs bootflags.$C bp.$C cat.$Cs catman.$C ccompress.$C \
	cdump.$Cs chown.$C \
	clri.$C comsat.$Cc config.$C copy.$Cs copy2.$Cs crash.$C cron.$C \
	dcheck.$C diskpart.$C dmesg.$C drtest.$Cs dump.$C dump.$Cs dumpfs.$C\
	edquota.$C fastboot.$C format.$C formatscsi.$Cs fsck.$C\
	fsck.$Cs fsdb.$C fsirand.$C fsstat.$C\
	ftpd.$Cc gettable.$Cc getty.$C halt.$C htable.$C icheck.$C\
	ifconfig.$Cc inetd.$Cc init.$C installboot.$C\
	intro.$C lpc.$C lpd.$C ls.$Cs\
	makedev.$C makekey.$C mkfs.$C mklost+found.$C mknod.$C\
	mkproto.$C mkvtoc.$C monitor.$C mount.$C\
	named.$C ncheck.$C newfs.$C noage.$C\
	online.$C pac.$C ping.$C pmap.$C\
	portmap.$Cc prtvtoc.$C prtvtoc.$Cs pstat.$C quot.$C\
	quotacheck.$C quotaon.$C rc.$C rdump.$Cc reboot.$C\
	renice.$C repquota.$C restore.$C ret.$Cs rexecd.$Cc rlogind.$Cc\
	rmt.$Cc route.$Cc routed.$Cc rpcinfo.$C rquotad.$Cc rmvtoc.$C\
	rmvtoc.$Cs\
	rrestore.$Cc rshd.$Cc rstatd.$Cc rusersd.$Cc rwalld.$Cc rwhod.$Cc\
	sa.$C savecore.$C sdformat.$Cs sendmail.$C sestat.$C showcfg.$C\
	shutdown.$C spray.$C sprayd.$Cc sticky.$C swapon.$C sweepd.$C\
	sync.$C syslogd.$C talkd.$Cc telnetd.$Cc tftpd.$Cc\
	timed.$C timedc.$C trpt.$Cc ttyconfig.$C tunefs.$C unccompress.$C\
	update.$C usclk_conf.$C uucico.$Cc uuclean.$Cc uupoll.$Cc\
	uusnap.$Cc uuxqt.$Cc version.$C vipw.$C vmtune.$C\
	wdformat.$Cs xpformat.$Cs zdformat.$Cs

STRIPE_PGS = newst.$C putst.$C getst.$C

MIRROR_PGS = mirror.$C mrinval.$C

# If you wondered what happened to ypwhich.8, it never existed
# according to Anne Ruggles at Sun.

.8.Z .8c.Zc .8s.Zs	:;
	cd ..; umask 111; nroff $(MANMACS) man8/$< | compress > $M/$<.Z
.8.P .8c.Pc .8s.Ps	:;
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:	mirror_install stripe_install
	install -c -m 444 $(PAGES) $(DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z pages

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

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages
stripe_print	:
	 /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P stripe_pages
mirror_print	:
	 /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P mirror_pages

# Specials (for tbl processing)
pstat.Z:
	cd ..; tbl < man8/pstat.8 > man8/pstat.8.tbl ; \
	umask 111; nroff $(MANMACS) man8/pstat.8.tbl | compress > $M/pstat.8.Z
prtvtoc.Z: prtvtoc.8
	umask 111; tbl $? | nroff -i $(MANMACS) | compress >$M/prtvtoc.8.Z
prtvtoc.Zs: prtvtoc.8s
	umask 111; tbl $? | nroff -i $(MANMACS) | compress >$M/prtvtoc.8s.Z

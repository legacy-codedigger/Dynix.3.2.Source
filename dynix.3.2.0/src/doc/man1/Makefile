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

# $Header: Makefile 1.92 1991/06/12 00:54:42 $
#
.SUFFIXES:
.SUFFIXES: .1 .1c .1g .Z .Zc .Zg .P .Pc .Pg .C .Cc .Cg

M	= $(DESTDIR)/usr/man/cat1
D	= $(CRYPT_DESTDIR)/usr/man/cat1
C	= 1
ITPF	= /usr/imagen/bin/itpf -x

# The following macro file has extras for hard-copy printing and goes
#	with Sequent techpubs version of tmac.an only.
# All of this is to get a consistent date at the bottom of each page.

TPMACS	= man.macs

MANMACS	= $(DESTDIR)/usr/lib/tmac/tmac.an

CPAGES	= crypt.$C xsend.$C

PAGES	=\
	4.2talk.$C a.tags.$C a.vi.$C addbib.$C apply.$C apropos.$C ar.$C as.$C\
	at.$C awk.$C basename.$C bc.$C biff.$C binmail.$C btoa.$C cal.$C\
	calendar.$C cat.$C cb.$C cc.$C cd.$C checknr.$C chfn.$C chgrp.$C\
	chmod.$C chsh.$C ci.$C clear.$C cmp.$C co.$C col.$C colcrt.$C\
	colrm.$C comm.$C compact.$C compress.$C\
	compressdir.$C cp.$C cpp.$C\
	csh.$C ctags.$C date.$C dc.$C dd.$C ddt.$C deroff.$C df.$C\
	diction.$C diff.$C diff3.$C disas.$C domainname.$C du.$C echo.$C\
	ed.$C eqn.$C error.$C ex.$C expand.$C explain.$C expr.$C\
	eyacc.$C false.$C fed.$C file.$C find.$C finger.$C fmt.$C\
	fold.$C fpr.$C from.$C fsplit.$C ftp.$Cc gcore.$C gprof.$C\
	graph.$Cg grep.$C groups.$C head.$C host.$C hostid.$C\
	hostname.$C id.$C ident.$C indent.$C install.$C intro.$C\
	iostat.$C join.$C kill.$C ksh.$C last.$C lastcomm.$C ld.$C learn.$C\
	leave.$C lex.$C lint.$C lisp.$C ln.$C lock.$C logger.$C login.$C\
	look.$C lookbib.$C lorder.$C lpq.$C lpr.$C lprm.$C ls.$C m4.$C\
	machid.$C mail.$C\
	mailbug.$C make.$C man.$C merge.$C mesg.$C mkdir.$C mkstr.$C\
	more.$C msgs.$C mt.$C mv.$C netstat.$C newaliases.$C nice.$C\
	nm.$C nroff.$C nslookup.$C od.$C pagesize.$C passwd.$C pc.$C pdbx.$C\
	plot.$Cg pmerge.$C pr.$C print.$C printenv.$C prmail.$C prof.$C\
	ps.$C pti.$C ptx.$C pwd.$C pxp.$C quota.$C ranlib.$C rcp.$Cc rcs.$C\
	rcsdiff.$C rcsintro.$C rcsmerge.$C rdist.$C refer.$C reset.$C rev.$C\
	rlog.$C rlogin.$Cc rm.$C rmail.$C rmdir.$C roffbib.$C rsh.$Cc\
	rup.$Cc ruptime.$Cc rusers.$Cc rwho.$Cc sccs.$C script.$C\
	sed.$C sh.$C size.$C sleep.$C soelim.$C sort.$C sortbib.$C spell.$C\
	spline.$Cg split.$C strings.$C strip.$C struct.$C stty.$C\
	style.$C su.$C sum.$C symorder.$C sysline.$C tabs.$C tail.$C\
	talk.$C tar.$C tbl.$C tc.$C tee.$C telnet.$Cc test.$C tftp.$Cc\
	time.$C tip.$Cc tk.$C touch.$C tp.$C tr.$C troff.$C\
	true.$C tset.$C tsort.$C tty.$C ul.$C unifdef.$C uniq.$C\
	units.$C universe.$C uptime.$C users.$C uucp.$Cc uuencode.$Cc\
	uulog.$Cc uuname.$Cc uuq.$Cc uusend.$Cc uux.$Cc vfontinfo.$C\
	vgrind.$C vi.$C vmstat.$C vpr.$C vtroff.$C vwidth.$C w.$C\
	wait.$C wall.$C wc.$C what.$C whatis.$C whereis.$C which.$C\
	who.$C whoami.$C whois.$C write.$C xargs.$C xstr.$C yacc.$C yes.$C\
	zcmp.$C zmore.$C

.1.C .1c.Cc .1g.Cg:
	cd ..; umask 111; nroff $(MANMACS) man1/$< | compress > $D/$<.Z
.1.Z .1c.Zc .1g.Zg:
	cd ..; umask 111; nroff $(MANMACS) man1/$< | compress > $M/$<.Z
.1.P .1c.Pc .1g.Pg:
	$(ITPF) -M$(MANMACS) ../$(TPMACS) $<

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install	:; sh -c 'if [ "$(EXPORT)" = TRUE ]; then :; else \
		install -c -m 444 $(CPAGES) $(CRYPT_DESTDIR)/usr/man/man$C/;\
		make $(MFLAGS) $(MRULES) C=C cpages; \
	fi'
	install -c -m 444 $(PAGES) $(DESTDIR)/usr/man/man$C/
	make $(MFLAGS) $(MRULES) C=Z pages
	install    -m 644 eqn.1.eqn $(DESTDIR)/usr/man/man$C/eqn.1
	install -c -m 644 eqn.1 $(DESTDIR)/usr/man/man$C/eqn.1.eqn
	install    -m 644 spline.1g.eqn $(DESTDIR)/usr/man/man$C/spline.1g
	install -c -m 644 spline.1g $(DESTDIR)/usr/man/man$C/spline.1g.eqn
	install    -m 644 ddt.1.tbl $(DESTDIR)/usr/man/man$C/ddt.1
	install -c -m 644 ddt.1 $(DESTDIR)/usr/man/man$C/ddt.1.tbl
	install    -m 644 fpr.1.tbl $(DESTDIR)/usr/man/man$C/fpr.1
	install -c -m 644 fpr.1 $(DESTDIR)/usr/man/man$C/fpr.1.tbl
	install    -m 644 ld.1.tbl $(DESTDIR)/usr/man/man$C/ld.1
	install -c -m 644 ld.1 $(DESTDIR)/usr/man/man$C/ld.1.tbl
	install    -m 644 tbl.1.tbl $(DESTDIR)/usr/man/man$C/tbl.1
	install -c -m 644 tbl.1 $(DESTDIR)/usr/man/man$C/tbl.1.tbl
	if [ "$(MACHINE)" != "ns32000" ]; then \
		rm -f $(DESTDIR)/usr/man/man$C/lisp.1 $M/lisp.1.Z; \
	else \
		:; \
	fi
	touch $M/eqn.1.Z $M/spline.1g.Z $M/ddt.1.Z $M/fpr.1.Z $M/ld.1.Z \
		$M/tbl.1.Z

pages	:& $(PAGES)
cpages	:& $(CPAGES)

# "ccsmake" required because tools directory is in techpubs view
print	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P pages
cprint	:; /ccsbin/ccsmake $(MFLAGS) TOOLS=/ccs/techpubs/tools C=P cpages

# Specials (for eqn processing)
eqn.Z:
	cd ..; neqn man1/eqn.1 > man1/eqn.1.eqn ; \
	umask 111; nroff $(MANMACS) man1/eqn.1.eqn | compress > $M/eqn.1.Z
spline.Zg:
	cd ..; neqn < man1/spline.1g > man1/spline.1g.eqn ; \
	umask 111; nroff $(MANMACS) man1/spline.1g.eqn | compress > $M/spline.1g.Z

# Specials (for tbl processing)
ddt.Z:
	cd ..; tbl < man1/ddt.1 > man1/ddt.1.tbl ; \
	umask 111; nroff $(MANMACS) man1/ddt.1.tbl | compress > $M/ddt.1.Z
fpr.Z:
	cd ..; tbl < man1/fpr.1 > man1/fpr.1.tbl ; \
	umask 111; nroff $(MANMACS) man1/fpr.1.tbl | compress > $M/fpr.1.Z
ld.Z:
	cd ..; tbl < man1/ld.1 > man1/ld.1.tbl ; \
	umask 111; nroff $(MANMACS) man1/ld.1.tbl | compress > $M/ld.1.Z
tbl.Z:
	cd ..; tbl < man1/tbl.1 > man1/tbl.1.tbl ; \
	umask 111; nroff $(MANMACS) man1/tbl.1.tbl | compress > $M/tbl.1.Z

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

# $Header: Makefile 1.30 1991/08/08 21:07:37 $

# man pages

PRINTER=im4
ROFF=/usr/local/bin/itpf -x -P$(PRINTER)
LAST_BASE=v3.1.0
CCS=/ccs/dynix/$(LAST_BASE)/work/src/doc

MANDIRS = man0 man1 man2 man3 man4 man5 man6 man7 man8

# used documentation

DOCDIRS = usr.doc Mail adv.ed cacm config csh curses diction edtut eqn ex \
	fs fsck gprof ipc lisp lpd memacros misc msmacros refer \
	 sendmail shell sysman troff trofftut uchanges uprog uucp xdr yacc

# Don't forget to add pascal (to DOCDIRS) when it is ready.

# unused documentation

DOCUDIRS = as beginners berknet ctour diskperf f77 fp hints kchanges net \
	porttour ratfor sdb setup summary

all:

# install will pick up mirror_install at lower make files

install: install.man install.usr.doc install.rnotes

whatis: mirror_whatis stripe_whatis FRC
	-@if [ -f $(DESTDIR)/usr/lib/makewhatis ]; then \
		echo "Installing $(DESTDIR)/usr/lib/whatis"; \
		DESTDIR=$(DESTDIR) sh $(DESTDIR)/usr/lib/makewhatis; \
	fi; \
	exit 0

stripe_whatis: FRC
	-@if [ -f $(DESTDIR)/usr/lib/makewhatis ]; then \
		echo "Installing $(STRIPE_DESTDIR)/usr/lib/whatis.stripe"; \
		DESTDIR=$(STRIPE_DESTDIR) sh $(DESTDIR)/usr/lib/makewhatis; \
		mv $(STRIPE_DESTDIR)/usr/lib/whatis \
			$(STRIPE_DESTDIR)/usr/lib/whatis.stripe ; \
	fi; \
	exit 0

mirror_whatis: FRC
	-@if [ -f $(DESTDIR)/usr/lib/makewhatis ]; then \
		echo "Installing $(MIRROR_DESTDIR)/usr/lib/whatis.mirror"; \
		DESTDIR=$(MIRROR_DESTDIR) sh $(DESTDIR)/usr/lib/makewhatis; \
		mv $(MIRROR_DESTDIR)/usr/lib/whatis \
			$(MIRROR_DESTDIR)/usr/lib/whatis.mirror ; \
	fi; \
	exit 0

install.man:&		$(MANDIRS)
	install -c -m 644 man.aliases $(DESTDIR)/usr/man/aliases

install.usr.doc:&	$(DOCDIRS)

install.rnotes:&	RELEASE.NOTES
	install -c -m 400 RELEASE.NOTES $(DESTDIR)/usr/RELEASE.NOTES
	compress $(DESTDIR)/usr/RELEASE.NOTES

$(MANDIRS) $(DOCDIRS): FRC
	cd $@; make $(MFLAGS) $(MRULES) -P1 install

stripe_install: stripe_whatis
	for i in $(MANDIRS) $(DOCDIRS) ; \
	do if grep -s 'stripe_install.*:' $$i/Makefile ; \
		then make $(MFLAGS) $(MRULES) $$i ; \
		fi ; \
	done

mirror_install: mirror_whatis
	for i in $(MANDIRS) $(DOCDIRS) ; \
	do if grep -s 'mirror_install.*:' $$i/Makefile ; \
		then make $(MFLAGS) $(MRULES) $$i ; \
		fi ; \
	done

# sequent local only
changes:
	rm -f tmp tmp1 tmp2 tmp3
	for P in `grep "^put.*man[1-8]" $(CCSVIEW)/../etc/log | sed -e \
		's/.*doc.//' -e 's/,v//' -e '/Makefile/d' | sort -u ` ;\
	do \
		echo $$P >>tmp2; \
		if [ -f $(CCS)/$$P ] ; then \
			/usr/local/bin/diffmk $(CCS)/$$P $$P tmp; \
			cat tmp >>tmp1; \
		else \
			cat $$P >>tmp3; \
		fi \
	done
	$(ROFF) -man tmp1
	$(ROFF) -man tmp3

# sequent local only
one_change:
	/usr/local/bin/diffmk $(CCS)/$(PAGE) $(PAGE) tmp; \
	$(ROFF) -man tmp

clean:

FRC:

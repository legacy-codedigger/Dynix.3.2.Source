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

# $Header: Makefile 1.23 87/07/30 $
#
TPFLAGS	= ../man.macs
ITPF	= /usr/imagen/bin/itpf -x
YTPF	= /usr/imagen/bin/itpf -y
TPFLAGS	= ../man.macs
TMACTOC	= /usr/imagen/lib/tmac/tmac.toc

INSTDIR	= $(DESTDIR)/usr/doc/config

SRC	= article.t 0.t 1.t 2.t 3.t 4.t 5.t 6.t a.t b.t c.t d.t

CTARGETS = 0.diff 1.diff 2.diff 3.diff 4.diff 5.diff 6.diff\
	a.diff b.diff c.diff d.diff

MTARGETS = 0.old 1.old 2.old 3.old 4.old 5.old 6.old\
	a.old b.old c.old d.old

source	:& $(SRC)

.SUFFIXES:
.SUFFIXES: .diff .old .raw .t .toc .P .Y

toc.P	:toc.toc
	$(ITPF) -M$(TMACTOC) toc.toc

toc.Y	:toc.toc
	$(YTPF) -M$(TMACTOC) toc.toc

.t.P	:;
	$(ITPF) $(TPFLAGS) $<

.t.Y	:;
	$(YTPF) $(TPFLAGS) $<

.t.old	:&
	mv $< $*.old

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

toc.toc	:; @echo "Sorry, you must create toc.toc by hand."; exit 1

install	:; install -c -m 444 $(SRC) $(INSTDIR)
#	install -c -m 444 Makefile $(INSTDIR)/Makefile

print	: article.P toc.P

change	: Article.P

final	: article.Y toc.Y

# "make toc.raw" collects all the .Ct and .Ah macros for the book
# table of contents (toc.toc).
toc.raw	:
	egrep '^.Ct|^.Ah|^.Bh' $(SRC) | sed -e "s/^.*:\./\./" > toc.raw

clean	:& $(MTARGETS)
	rm -f $(CTARGETS)
	rm -f toc.raw

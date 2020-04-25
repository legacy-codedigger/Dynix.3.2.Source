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

# $Header: Makefile 2.0 86/01/28 $
#
# Makefile for sendmail documentation
#

OBJS=	intro.lpr op.lpr usenix.lpr
DOCS=	intro.me  op.me  usenix.me
SRCS=	$(DOCS) sendmail.8 syslog.3 syslog.8
ALL=	$(OBJS) rfc819.lpr rfc821.lpr rfc822.lpr

LPR=	lpr
TROFF=	vtroff
GET=	sccs get

.SUFFIXES: .me .lpr

.me.lpr:
	nroff -me -Tlp $< > $@

troff: $(DOCS)
	for i in $(DOCS); do $(TROFF) -me $$i; done

all: $(ALL)

print: $(ALL)
	$(LPR) $(ALL)

sources: $(SRCS)

$(SRCS):
	$(GET) $(REL) SCCS/s.$@

clean:
	rm -f $(OBJS) a.out core

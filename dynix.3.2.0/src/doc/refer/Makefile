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

# $Header: Makefile 1.13 86/05/20 $
#
TPFLAGS	= -rP1
TPFILES	= ../liberal
RFLAGS	= -p biblio
REFERN	= rsh crg0 "refer"
STAGE	= $(STAGEROOT)/vol2/refer
INSTDIR	= $(DESTDIR)/usr/doc/refer
SRC	= README biblio mx.doc pubuse refer refer.bib refer.mac runoff
TARGETS	= refer.F pubuse.F refer.bib.F
PRINT	= refer.P pubuse.P refer.bib.P

default		:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1
install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile
format		:& $(TARGETS)
refer.F		:refer; cat $* | $(REFERN) | $(TPF) -M$(TMACMSB) $(TPFILES) - \
		> $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@
pubuse.F	:pubuse; $(TPF) -M$(TMACMSB) $(TPFLAGS) $(TPFILES) $* \
		> $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@
refer.bib.F	:biblio refer.bib; rcp biblio crg0:; cat refer.bib | $(REFER) | \
		$(TPF) -M$(TMACMS) $(TPFILES) - > $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		:& $(PRINT)
clean		:; rm -f *.F $(STAGE)/*.F; /usr/local/crg0 rm biblio

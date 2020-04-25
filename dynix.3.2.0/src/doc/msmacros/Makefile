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

# $Header: Makefile 1.10 86/05/20 $
#
TPFLAGS	= ../liberal
STAGE	= $(STAGEROOT)/vol2/msmacros
INSTDIR	= $(DESTDIR)/usr/doc/msmacros
SRC	= ms refcard ms.diffs
STDTRGS	= ms.F refcard.F
TARGETS	= $(STDTRGS) ms.diffs.F
PRINT	= ms.P refcard.P ms.diffs.P

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1
install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile
format		:& $(TARGETS)
$(STDTRGS)	:; $(TPF) -M$(TMACMSB) $(TPFLAGS) $* > $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@
ms.F		:ms
refcard.F	:refcard
ms.diffs.F	:ms.diffs; $(TPF) -M$(TMACMS) $(TPFLAGS) $* > $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		:& $(PRINT)
clean		:; rm -f *.F $(STAGE)/*.F

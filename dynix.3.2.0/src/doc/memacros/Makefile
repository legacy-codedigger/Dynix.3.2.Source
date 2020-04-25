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
TPFLAGS	= -rv1 ../liberal
STAGE	= $(STAGEROOT)/vol2/memacros
INSTDIR	= $(DESTDIR)/usr/doc/memacros
SRC	= intro.nr ref.nr
TARGETS	= intro.F ref.F
PRINT	= intro.P ref.P

.SUFFIXES: .nr .F
.nr.F		:; $(TPF) -M$(TMACME) $(TPFLAGS) $< > $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1
install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile
format		:& $(TARGETS)
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		:& $(PRINT)
clean		:; rm -f *.F $(STAGE)/*.F

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

# $Header: Makefile 1.11 86/05/21 $
#
TPFLAGS	= ../liberal
STAGE	= $(STAGEROOT)/vol2/uucp
INSTDIR	= $(DESTDIR)/usr/doc/uucp
SRC	= implement network
TARGETS	= network.F implement.F
PRINT	= network.P implement.P

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1
install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile
format		:& $(TARGETS)
$(TARGETS)	:;cat $* | $(REFER) | $(TPF) -M$(TMACMSB) $(TPFLAGS) - \
		> $(STAGE)/$@; rm -f $@; ln -s $(STAGE)/$@
network.F	:network
implement.F	:implement
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		: $(PRINT)
clean		:; rm -f *.F $(STAGE)/*.F

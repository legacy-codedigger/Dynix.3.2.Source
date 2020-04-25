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

# $Header: Makefile 1.9 86/05/20 $
#
TPFLAGS	= P.mac
STAGE	= $(STAGEROOT)/vol2/uprog
INSTDIR	= $(DESTDIR)/usr/doc/uprog
SRC	= cwscript p.mac p0 p1 p2 p3 p4 p5 p6 p8 p9
NSRC	= p0 p1 p2 p3 p4 p5 p6 p8 p9
TARGET	= uprog.F
PRINT	= uprog.P

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1
install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile
format		:$(TARGET)
# For xroff use P.mac and Cwscript instead of p.mac and cwscript
$(TARGET)	:$(NSRC); sed -f Cwscript $(NSRC) | $(TPF) -M$(TMACMSB) $(TPFLAGS) - > $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		: $(PRINT)
clean		:; rm -f $(TARGET) $(STAGE)/$(TARGET)

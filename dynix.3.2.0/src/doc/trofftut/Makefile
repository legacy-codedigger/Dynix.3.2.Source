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

# $Header: Makefile 1.11 86/05/20 $
#
TPFLAGS	= ../liberal 
STAGE	= $(STAGEROOT)/vol2/trofftut
INSTDIR	= $(DESTDIR)/usr/doc/trofftut
SRC	= $(NSRC) ttindex
NSRC	=\
	tt.mac tt00 tt01 tt02 tt03 tt04 tt05 tt06 tt07 tt08 tt09\
	tt10 tt11 tt12 tt13 tt14 ttack ttcharset
TARGET	= trofftut.F
PRINT	= trofftut.P

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1
install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile
format		:$(TARGET)
$(TARGET)	:$(NSRC); $(TPF) -M$(TMACMSB) $(TPFLAGS) $(NSRC) > $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		: $(PRINT)
clean		:; rm -f $(TARGET) $(STAGE)/$(TARGET)

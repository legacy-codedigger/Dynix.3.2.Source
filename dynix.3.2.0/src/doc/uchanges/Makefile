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
TPFLAGS	= ../liberal 
STAGE	= $(STAGEROOT)/vol2/uchanges
INSTDIR	= $(DESTDIR)/usr/doc/uchanges
SRC	= 0.t 1.t 2.t 3.t 4.t 5.t 6.t 7.t 8.t 9.t
TARGET	= uchanges.F
PRINT	= uchanges.P

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1
install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile
format		:$(TARGET)
$(TARGET)	:$(SRC); $(TPF) -M$(TMACMS) $(TPFLAGS) $(SRC) > $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		: $(PRINT)
clean		:; rm -f $(TARGET) $(STAGE)/$(TARGET)

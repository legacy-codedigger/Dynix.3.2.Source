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
STAGE	= $(STAGEROOT)/vol2/sysman
INSTDIR	= $(DESTDIR)/usr/doc/sysman
SRC	= \
	0.t 1.0.t 1.1.t 1.2.t 1.3.t 1.4.t 1.5.t 1.6.t 1.7.t \
	2.0.t 2.1.t 2.2.t 2.3.t 2.4.t 2.5.t a.t
TARGET	= sysman.F
PRINT	= sysman.P

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

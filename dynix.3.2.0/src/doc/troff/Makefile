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
TPFLAGS	= -M ../liberal m.mac
STAGE	= $(STAGEROOT)/vol2/troff
INSTDIR	= $(DESTDIR)/usr/doc/troff
SRC	= add m.mac m0 m0a m1 m2 m3 m4 m5 table1 table2 tprint
STDTRGS	= m0.F m0a.F m5.F table1.F table2.F add.F
TARGETS	= $(STDTRGS) m1.F
PRINT	= m0.P m0a.P m1.P m5.P table1.P table2.P add.P

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1
install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile
format		:& $(TARGETS)
$(STDTRGS)	:;$(TPF) $(TPFLAGS) $* > $(STAGE)/$@
		rm -f $@; ln -s $(STAGE)/$@
m0.F		:m.mac m0
m0a.F		:m.mac m0a
m1.F		:m.mac m1 m2 m3 m4; $(TPF) $(TPFLAGS) m1 m2 m3 m4 \
		> $(STAGE)/$@; rm -f $@; ln -s $(STAGE)/$@
m5.F		:m.mac m5
table1.F	:m.mac table1
table2.F	:m.mac table2
add.F		:m.mac add
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		:& $(PRINT)
clean		:; rm -f *.F $(STAGE)/*.F

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
TPFLAGS	= -rv1 ../liberal ../picmacs
STAGE	= $(STAGEROOT)/vol2/sendmail
INSTDIR	= $(DESTDIR)/usr/doc/sendmail
SRC	= intro.me op.me rfc819.lpr rfc821.lpr rfc822.lpr usenix.me
TARGETS	= intro.F op.F usenix.F
PRINT	= intro.P op.P usenix.P

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1
install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 Makefile $(INSTDIR)/Makefile
format		:& $(TARGETS)
mkstage		:; -mkdir $(STAGEROOT)/vol2 $(STAGE)
print		:& $(PRINT)
clean		:; rm -f *.F $(STAGE)/*.F

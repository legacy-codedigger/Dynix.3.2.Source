# $Copyright:	$
# Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
# Sequent Computer Systems, Inc.   All rights reserved.
#  
# This software is furnished under a license and may be used
# only in accordance with the terms of that license and with the
# inclusion of the above copyright notice.   This software may not
# be provided or otherwise made available to, or used by, any
# other person.  No title to or ownership of the software is
# hereby transferred.

# $Header: Makefile 1.15 91/01/08 $
#
TPFLAGS	= ../man.macs
IMAGEN	= /usr/imagen
IBIN	= $(IMAGEN)/bin
ITPF	= $(IBIN)/itpf -x
YTPF	= $(IBIN)/itpf -y
TMAC	= /usr/lib/tmac
TMACME	= $(TMAC)/tmac.e

INSTDIR	= $(DESTDIR)/usr/doc/curses

SRC	=\
	Master Spellit appen.A appen.B appen.C c_macros disclaim doc.I\
	doc.II doc.III doc.IV intro.0 intro.1 intro.2 intro.3 intro.4\
	intro.5 life.c macros macros.coms twinkle1.c twinkle2.c\
	win_st.c life.gr twinkle1.gr twinkle2.gr win_st.gr

PRINT	=\
	Master

#
# These .gr versions have already been generated and someone has checked
# them in as real source.  Let's not do this again, let's use the source.
#
#.SUFFIXES: .c .gr
#
#.c.gr:
#	sed "1,/Header:/d" $< | /usr/lib/vfontedpr | sed "/^[.']wh/d" |\
#	../Fontfix >$@

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 makefile $(INSTDIR)/makefile

print	:&; $(ITPF) -M$(TMACME) $(TPFLAGS) $(PRINT)

final	:&; $(YTPF) -M$(TMACME) $(TPFLAGS) $(PRINT)

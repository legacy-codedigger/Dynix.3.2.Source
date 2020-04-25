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

# $Header: Makefile 1.14 87/04/20 $
#
TPFLAGS	= ../man.macs
IMAGEN	= /usr/imagen
IBIN	= $(IMAGEN)/bin
ITPF	= $(IBIN)/itpf -x
YTPF	= $(IBIN)/itpf -y
TMAC	= /usr/lib/tmac
TMACMS	= $(TMAC)/tmac.s

INSTDIR	= $(DESTDIR)/usr/doc/ex

SRC	=\
	edit.tut edit.vindex\
	ex.rm ex.summary ex1.1-2.0 ex2.0-3.1 ex3.1-3.5\
	vi.apwh.ms vi.chars vi.in vi.summary

# Note: although the SRC assignment is is alphabetical order,
#	the TARGETS assignment is in the order of Volume II.

TARGETS	=\
	vi.in vi.apwh.ms vi.chars vi.summary\
	edit.tut ex.rm ex.summary ex3.1-3.5 ex2.0-3.1 ex1.1-2.0

default	:; @echo "Sorry, this makefile is not meant to be invoked\
without a target."; exit 1

install		:; install -c -m 444 $(SRC) $(INSTDIR)
#		install -c -m 444 makefile $(INSTDIR)/makefile



print	:&;
	$(ITPF) -M$(TMACMS) $(TPFLAGS) vi.in
	$(ITPF) -M$(TMACMS) $(TPFLAGS) vi.apwh.ms
	$(ITPF) -M$(TMACMS) $(TPFLAGS) vi.chars
	$(ITPF) -M$(TMACMS) $(TPFLAGS) vi.summary
	$(ITPF) -M$(TMACMS) $(TPFLAGS) edit.tut
	$(ITPF) -M$(TMACMS) $(TPFLAGS) ex.rm
	$(ITPF) -M$(TMACMS) $(TPFLAGS) ex.summary
	$(ITPF) -M$(TMACMS) $(TPFLAGS) ex3.1-3.5
	$(ITPF) -M$(TMACMS) $(TPFLAGS) ex2.0-3.1
	$(ITPF) -M$(TMACMS) $(TPFLAGS) ex1.1-2.0

final	:&;
	$(YTPF) -M$(TMACMS) $(TPFLAGS) vi.in
	$(YTPF) -M$(TMACMS) $(TPFLAGS) vi.apwh.ms
	$(YTPF) -M$(TMACMS) $(TPFLAGS) vi.chars
	$(YTPF) -M$(TMACMS) $(TPFLAGS) vi.summary
	$(YTPF) -M$(TMACMS) $(TPFLAGS) edit.tut
	$(YTPF) -M$(TMACMS) $(TPFLAGS) ex.rm
	$(YTPF) -M$(TMACMS) $(TPFLAGS) ex.summary
	$(YTPF) -M$(TMACMS) $(TPFLAGS) ex3.1-3.5
	$(YTPF) -M$(TMACMS) $(TPFLAGS) ex2.0-3.1
	$(YTPF) -M$(TMACMS) $(TPFLAGS) ex1.1-2.0
